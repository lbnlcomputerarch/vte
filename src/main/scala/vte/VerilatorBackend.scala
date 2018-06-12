// adapted from chisel3.iotesters.VerilatorBackend by Albert Chen

package vte

import java.io._
import java.nio.file.StandardCopyOption.REPLACE_EXISTING
import java.nio.file.{FileAlreadyExistsException, Files, Paths}

import chisel3._
import chisel3.core.BaseModule
import chisel3.internal.firrtl.Circuit
import firrtl.annotations.CircuitName
import firrtl.transforms._

import scala.sys.process.{ProcessBuilder, _}

object getTopModule {
  def apply(circuit: Circuit): BaseModule = {
    (circuit.components find (_.name == circuit.name)).get.id
  }
}

/**
  * Copies the necessary header files used for verilator compilation to the specified destination folder
  */
object copyVerilatorHeaderFiles {
  def apply(destinationDirPath: String): Unit = {
    new File(destinationDirPath).mkdirs()
    val bitsHFilePath = Paths.get(destinationDirPath + "/bits.h")
    val bitsCppFilePath = Paths.get(destinationDirPath + "/bits.cpp")
    val testbenchHFilePath = Paths.get(destinationDirPath + "/testbench.h")
    val veriAggregateApiHFilePath = Paths.get(destinationDirPath + "/veri_aggregate_api.h")
    val veriApiHFilePath = Paths.get(destinationDirPath + "/veri_api.h")
    try {
      Files.createFile(bitsHFilePath)
      Files.createFile(bitsCppFilePath)
      Files.createFile(testbenchHFilePath)
      Files.createFile(veriAggregateApiHFilePath)
      Files.createFile(veriApiHFilePath)
    } catch {
      case _: FileAlreadyExistsException =>
        System.out.format("")
      case x: IOException =>
        System.err.format("createFile error: %s%n", x)
    }

    val rootDirPath = new File(".").getAbsolutePath()
    
    val bitsHFilePathSrc = Paths.get(rootDirPath + "/vte/src/main/cpp" + "/bits.h")
    val bitsCppFilePathSrc = Paths.get(rootDirPath + "/vte/src/main/cpp" + "/bits.cpp")
    val testbenchHFilePathSrc = Paths.get(rootDirPath + "/vte/src/main/cpp" + "/testbench.h")
    val veriAggregateApiHFilePathSrc = Paths.get(rootDirPath + "/vte/src/main/cpp" + "/veri_aggregate_api.h")
    val veriApiHFilePathSrc = Paths.get(rootDirPath + "/vte/src/main/cpp" + "/veri_api.h")

    Files.copy(bitsHFilePathSrc, bitsHFilePath, REPLACE_EXISTING);
    Files.copy(bitsCppFilePathSrc, bitsCppFilePath, REPLACE_EXISTING)
    Files.copy(testbenchHFilePathSrc, testbenchHFilePath, REPLACE_EXISTING)
    Files.copy(veriAggregateApiHFilePathSrc, veriAggregateApiHFilePath, REPLACE_EXISTING)
    Files.copy(veriApiHFilePathSrc, veriApiHFilePath, REPLACE_EXISTING)
  }
}

object setupVerilatorBackend {

  def verilogToCpp(
                    dutFile: String,
                    dir: File,
                    vSources: Seq[File],
                    cppHarness: File,
                    testbenchCppFile: File
                  ): ProcessBuilder = {
    val topModule = dutFile

    val blackBoxVerilogList = {
      val list_file = new File(dir, firrtl.transforms.BlackBoxSourceHelper.FileListName)
      if (list_file.exists()) {
        Seq("-f", list_file.getAbsolutePath)
      }
      else {
        Seq.empty[String]
      }
    }

    val command = Seq(
      "verilator",
      testbenchCppFile.getAbsolutePath,
      s"${dir.getAbsolutePath}/bits.cpp",
      "--cc", s"${dir.getAbsolutePath}/$dutFile.v"
    ) ++
      blackBoxVerilogList ++
      vSources.flatMap(file => Seq("-v", file.getAbsolutePath)) ++
      Seq("--assert",
        "-Wno-fatal",
        "-Wno-WIDTH",
        "-Wno-STMTDLY",
        "--trace",
        "-O1",
        "--top-module", topModule,
        "+define+TOP_TYPE=V" + dutFile,
        s"+define+PRINTF_COND=!$topModule.reset",
        s"+define+STOP_COND=!$topModule.reset",
        "-CFLAGS",
        s"""-std=c++11 -Wno-undefined-bool-conversion -pedantic -O1 -DTOP_TYPE=V$dutFile -DVL_USER_FINISH -include V$dutFile.h""",
        "--compiler", "clang",
        "-Mdir", dir.getAbsolutePath,
        "--exe", cppHarness.getAbsolutePath)
    System.out.println(s"${command.mkString(" ")}") // scalastyle:ignore regex
    command
  }

  def apply[T <: chisel3.Module](dutGen: () => T, optionsManager: TesterOptionsManager): T = {
    import firrtl.{ChirrtlForm, CircuitState}

    optionsManager.makeTargetDir()

    optionsManager.chiselOptions = optionsManager.chiselOptions.copy(
      runFirrtlCompiler = false
    )
    val dir = new File(optionsManager.targetDirName)

    // Generate CHIRRTL
    chisel3.Driver.execute(optionsManager, dutGen) match {
      case ChiselExecutionSuccess(Some(circuit), emitted, _) =>

        val chirrtl = firrtl.Parser.parse(emitted)
        val dut = getTopModule(circuit).asInstanceOf[T]

        // This makes sure annotations for command line options get created
        firrtl.Driver.getAnnotations(optionsManager)

        val annotations = optionsManager.firrtlOptions.annotations ++
          List(BlackBoxTargetDirAnno(optionsManager.targetDirName))

        //val transforms = optionsManager.firrtlOptions.customTransforms
        copyVerilatorHeaderFiles(optionsManager.targetDirName)
        FirrtlToCppAST(chirrtl)

        // Generate Verilog
        val verilogFile = new File(dir, s"${circuit.name}.v")
        val verilogWriter = new FileWriter(verilogFile)

        val compileResult = (new firrtl.VerilogCompiler).compileAndEmit(
          CircuitState(chirrtl, ChirrtlForm, annotations)
        )
        val compiledStuff = compileResult.getEmittedCircuit
        verilogWriter.write(compiledStuff.value)
        verilogWriter.close()

        val codeGen = new VerilatorTestbenchGenerator(chirrtl)

        // Generate Testbench header
        val testbenchHeaderFileName = s"${circuit.name}_testbench.h"
        val testbenchHeaderFile = new File(dir, testbenchHeaderFileName)
        val testbenchHeaderWriter = new FileWriter(testbenchHeaderFile)
        val testbenchHeaderCode = codeGen.testbenchHeaderGen()
        testbenchHeaderWriter.append(testbenchHeaderCode)
        testbenchHeaderWriter.close()

        // Generate empty testbench::run() file unless one already exists
        val testbenchCppFileName = if (optionsManager.testerOptions.testbenchCppFile.isEmpty) {
          s"${dir.getAbsolutePath}/${circuit.name}_testbench.cpp"
        } else {
          optionsManager.testerOptions.testbenchCppFile
        }
        val testbenchCppFile = new File(testbenchCppFileName)
        if (!Files.exists(Paths.get(testbenchCppFileName))) {
          val testbenchCppWriter = new FileWriter(testbenchCppFile)
          val testbenchCppCode = codeGen.testbenchCppGen()
          testbenchCppWriter.append(testbenchCppCode)
          testbenchCppWriter.close()
        }

        // Generate Main
        val mainFileName = s"${circuit.name}_main.cpp"
        val mainFile = new File(dir, mainFileName)
        val mainWriter = new FileWriter(mainFile)
        val mainStuff = codeGen.mainGen()
        mainWriter.append(mainStuff)
        mainWriter.close()

        assert(
          verilogToCpp(
            circuit.name,
            dir,
            vSources = Seq(),
            mainFile,
            testbenchCppFile
          ).! == 0
        )

        dut
    }
  }
}
