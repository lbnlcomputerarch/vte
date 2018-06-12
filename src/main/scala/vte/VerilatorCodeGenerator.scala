// written by Albert Chen

package vte

import firrtl.ir.Circuit

import scala.collection.mutable

class VerilatorTestbenchGenerator(val circuit: Circuit) {
  final val dutName: String = circuit.main
  final val cppAST: BundleCppAST = FirrtlToCppAST(circuit)
  final val bundleTypeIndexMap: Map[CppASTNode, Int] = indexBundles(cppAST)

  private def indexBundles(ast: CppASTNode): Map[CppASTNode, Int] = {
    val bundleSets = new mutable.ListBuffer[mutable.Set[CppASTNode]]()

    ast foreachPostOrderDepthFirst(
      node => bundleSets.find( set => set.head.sameTypeAs(node) ) match {
        case Some(s) => s.add(node)
        case None => val newSet = new mutable.HashSet[CppASTNode]()
          newSet += node
          bundleSets += newSet
      }
    )

    bundleSets.zipWithIndex.flatMap {
      case (set, index) => set map (node => node -> index)
    }(collection.breakOut)
  }

  private def getVerilatorClassName(data: CppASTNode): String = {
    data match {
      case w: WireCppAST =>
          if (w.width <= 8) {
            "VerilatorCData"
          } else if (w.width <= 16) {
            "VerilatorSData"
          } else if (w.width <= 32) {
            "VerilatorIData"
          } else if (w.width <= 64) {
            "VerilatorQData"
          } else {
            "VerilatorWData"
          }
      case b: BundleCppAST => s"VerilatorBundle${bundleTypeIndexMap(b)}"
      case v: VecCppAST => s"VerilatorVec<${getVerilatorClassName(v.children.head)}>"
    }
  }

  private def makeVerilatorInstantiation(instanceName: String, data: CppASTNode): String = {
    val args = data match {
      case w: WireCppAST => Seq(w.instanceName)
      case b: BundleCppAST => b.wires map (_.instanceName)
      case v: VecCppAST => v.children.head match {
        case _: WireCppAST => v.children map(_.instanceName)
        case _: BundleCppAST => v.children map(
          child => child.asInstanceOf[BundleCppAST].wires.map(wire => wire.instanceName).addString(new StringBuilder(),
            start=s"VerilatorBundle${bundleTypeIndexMap(child)}{",
            sep=", ",
            end="}")
          )
        case _: VecCppAST => v.children map(child => makeVerilatorInstantiation(getVerilatorClassName(child), child))
      }
    }

    args.addString(new StringBuilder, start = s"${instanceName}(", sep = ", ", end = ")").mkString
  }

  def makeBundleClass(codeBuffer: StringBuilder, bundle: BundleCppAST) {
    val className = getVerilatorClassName(bundle)
    val elements = (bundle.fields map { case (n, d) => (n, d, getVerilatorClassName(d)) }) filter {
      _._3.nonEmpty
    }
    val verilatorBundleClassName = "VerilatorBundle"

    codeBuffer.append(s"class $className : public $verilatorBundleClassName {\n")

    // define set_pointers method for initializing element map and order list
    codeBuffer.append("private:\n")
    codeBuffer.append(s"    void set_pointers() {\n")
    codeBuffer.append(s"        first_wire = &${elements.last._1};\n\n")
    elements foreach { case (name, data, elementClassName) =>
      codeBuffer.append(s"""        elements[\"$name\"] = &$name;\n""")
      codeBuffer.append(s"        order.push_back((VerilatorDataWrapper *) &$name);\n\n")
    }
    codeBuffer.append("    }\n\n")

    // list public fields
    codeBuffer.append("public:\n")
    elements foreach { case (name, data, elementClassName) =>
      codeBuffer.append(s"    $elementClassName $name;\n")
    }

    // make constructor
    (bundle.wires map { case d => s"${getVerilatorClassName(d)} ${d.instanceName}" })
      .addString(codeBuffer,
        start = s"    $className(\n        ",
        sep = s",\n        ",
        end = ") :\n")
    elements map {
      case (name, data, dontCare) => makeVerilatorInstantiation(name, data)
    } addString(codeBuffer,
      start = "            ",
      sep = s",\n            ",
      end = " {\n")
    codeBuffer.append("        set_pointers();\n")
    codeBuffer.append("    }\n\n")


    // make copy constructor
    codeBuffer.append(s"    $className($className const &val) :\n")
    elements map {
      case (name, data, dontCare) => s"$name(val.$name)"
    } filter (_.nonEmpty) addString(codeBuffer,
      start = "            ",
      sep = s",\n            ",
      end = " {\n")
    codeBuffer.append("        set_pointers();\n")
    codeBuffer.append("    }\n")
    codeBuffer.append("};\n\n")
  }

  def testbenchHeaderGen(): String = {
    val codeBuffer = new StringBuilder
    val dutVerilatorClassName = "V" + dutName
    val testbenchName = dutName + "_testbench"
    codeBuffer.append(s"""#ifndef ${testbenchName.capitalize}_H\n""")
    codeBuffer.append(s"""#define ${testbenchName.capitalize}_H\n\n""")

    codeBuffer.append("#include \"%s.h\"\n".format(dutVerilatorClassName))
    codeBuffer.append("#include \"veri_api.h\"\n")
    codeBuffer.append("#include \"veri_aggregate_api.h\"\n")
    codeBuffer.append("#include \"testbench.h\"\n\n")

    //makeVerilatorClassDefinitions(codeBuffer, cppAST)
    val seenBundleTypes = new mutable.HashSet[Int]()
    cppAST foreachPostOrderDepthFirst {
      case b: BundleCppAST =>
        val typeIndex = bundleTypeIndexMap(b)
        if (!seenBundleTypes.contains(typeIndex)) {
        makeBundleClass(codeBuffer, b)
        seenBundleTypes += typeIndex
      }
      case _ => Unit
    }

    codeBuffer.append(s"class $testbenchName : public Testbench<$dutVerilatorClassName> {\n")

    val ioName = "io"

    // public members
    codeBuffer.append("public:\n")
    codeBuffer.append(s"    ${getVerilatorClassName(cppAST)} $ioName;\n")

    // constructor
    val constructorStart = s"""    $testbenchName(): $ioName("""
    cppAST.wires map {
      data => s"""${getVerilatorClassName(data)}("${data.instanceName}", ${data.width}, &dut->${data.instanceName})"""
    } addString (codeBuffer,
      start=constructorStart,
      sep=s",\n${" " * constructorStart.length}",
      end=") {\n")
    codeBuffer.append("    }\n\n")

    codeBuffer.append("    void run();\n")
    codeBuffer.append("};\n\n")
    codeBuffer.append("#endif")

    codeBuffer.toString()
  }

  def testbenchCppGen(): String = {
    val codeBuffer = new StringBuilder
    val testbenchName = s"${dutName}_testbench"
    codeBuffer.append("#include \"" + testbenchName + ".h\"\n\n")

    codeBuffer.append(s"void $testbenchName::run() {\n")
    codeBuffer.append("}\n")

    codeBuffer.toString()
  }

  def mainGen(): String = {
    val codeBuffer = new StringBuilder
    val testbenchName = s"${dutName}_testbench"
    codeBuffer.append(s"""#include "$testbenchName.h"\n\n""")

    codeBuffer.append("double sc_time_stamp() {\n")
    codeBuffer.append(s"    return $testbenchName::main_time;\n")
    codeBuffer.append("}\n\n")

    codeBuffer.append("int main(int argc, char **argv) {\n")
    codeBuffer.append(s"    $testbenchName tb;\n")

    codeBuffer.append("#if VM_TRACE\n")
    codeBuffer.append(s"""    std::string vcdfile = "$testbenchName.vcd";\n""")
    codeBuffer.append("    Verilated::traceEverOn(true);\n")
    codeBuffer.append("    VerilatedVcdC* tfp = new VerilatedVcdC;\n")
    codeBuffer.append("    tb.dut->trace(tfp, 99);\n")
    codeBuffer.append("    tfp->open(vcdfile.c_str());\n")
    codeBuffer.append("    tb.init_dump(tfp);\n")
    codeBuffer.append("#endif\n")

    codeBuffer.append("    tb.run();\n")
    codeBuffer.append("#if VM_TRACE\n")
    codeBuffer.append("    if (tfp) tfp->close();\n")
    codeBuffer.append("    delete tfp;\n")
    codeBuffer.append("#endif\n")
    codeBuffer.append("    tb.finish();\n")
    codeBuffer.append("}\n")

    codeBuffer.toString()
  }
}
