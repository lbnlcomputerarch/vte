// written by Albert Chen

package vte

import firrtl.ir._

object FirrtlToCppAST {

  def apply(c: Circuit): BundleCppAST = {
    getTopLevelCppAST((c.modules find (_.name == c.main)).get.ports)
  }

  def portToCppAST(name: String, tpe: Type): CppASTNode = {
    tpe match {
      case b: BundleType => getBundleCppAST(name, b)
      case v: VectorType => getVectorCppAST(name, v)
      case u: UIntType   => getUIntCppAST(name, u.width)
      case s: SIntType   => getUIntCppAST(name, s.width)
    }
  }

  def getTopLevelCppAST(ports: Seq[Port]): BundleCppAST = {
    val dataPorts = ports filterNot {
      case port => port.name == "reset" || port.name == "clock"
    }

    dataPorts.head.tpe match {
      case b: BundleType => getBundleCppAST("io", b)
    }
  }

  def getBundleCppAST(name: String, bundleType: BundleType): BundleCppAST = {
    val fields: Map[String, CppASTNode] = bundleType.fields.map({
      field => field.name -> portToCppAST(s"${name}_${field.name}", field.tpe)
    })(collection.breakOut)

    new BundleCppAST(name, fields)
  }

  def getVectorCppAST(name: String, vectorType: VectorType): VecCppAST = {
    val children = (0 until vectorType.size) map {
      i => portToCppAST(s"${name}_$i", vectorType.tpe)
    }

    new VecCppAST(name, children)
  }

  def getUIntCppAST(name: String, width: Width): WireCppAST = {
    val astWidth = width match {
      case i: IntWidth => i.width
      case UnknownWidth => sys.error("Unknown width")
    }

    new WireCppAST(name, astWidth)
  }
}

case class CppWireType(width: Width) extends GroundType {
  def serialize: String = width.serialize
  def mapWidth(f: Width => Width): Type = UIntType(f(width))
}

case class CppField(name: String, flip: Orientation, tpe: Type) extends FirrtlNode with HasName {
  def serialize: String = name + tpe.serialize
}

case class CppBundleType(fields: Seq[CppField]) extends AggregateType {
  def serialize: String = "Bundle" + (fields map (_.serialize) mkString ", ")
  def mapType(f: Type => Type): Type =
    CppBundleType(fields map (x => x.copy(tpe = f(x.tpe))))
}

case class CppVectorType(tpe: Type, size: Int) extends AggregateType {
  def serialize: String = s"VerilatorVec<${tpe.serialize}>"
  def mapType(f: Type => Type): Type = this.copy(tpe = f(tpe))
}


trait CppASTNode {
  val children: Seq[CppASTNode]
  val instanceName: String

  def sameTypeAs(other: CppASTNode): Boolean

  def foreachPreOrderDepthFirst[T](f: CppASTNode => T): Unit = {
    f(this)
    children.foreach (_.foreachPreOrderDepthFirst(f))
  }

  def foreachPreOrderBreadthFirst[T](f: CppASTNode => T): Unit = {
    f(this)
    children.foreach (_.foreachPreOrderBreadthFirst(f))
  }

  def foreachPostOrderDepthFirst[T](f: CppASTNode => T): Unit = {
    children.foreach (_.foreachPostOrderDepthFirst(f))
    f(this)
  }

  def foreachPostOrderBreadthFirst[T](f: CppASTNode => T): Unit = {
    children.foreach (_.foreachPostOrderBreadthFirst(f))
    f(this)

  }
}

object CppASTNode {
  final val indent: String = "  "
}

trait CppASTLeaf extends CppASTNode {
  override val children: Seq[CppASTNode] = Seq.empty
}

class BundleCppAST(val instanceName: String,
                   val fields: Map[String, CppASTNode]) extends CppASTNode {

  val children: Seq[CppASTNode] = fields.values.toSeq

  val wires: Seq[WireCppAST] = children flatMap {
    case w: WireCppAST => Seq(w)
    case b: BundleCppAST => b.wires
    case v: VecCppAST => v.wires
  } toIndexedSeq

  def equivalentTo(bundleCppAST: BundleCppAST): Boolean = {
    (fields.size == bundleCppAST.fields.size) &&
      ((fields zip bundleCppAST.fields) forall {
        i => i._1._1 == i._2._1 && i._1._2.sameTypeAs(i._2._2)
      })
  }

  def sameTypeAs(other: CppASTNode): Boolean = {
    other match {
      case b: BundleCppAST => equivalentTo(b)
      case _ => false
    }
  }
}

class VecCppAST(val instanceName: String,
                val children: Seq[CppASTNode]) extends CppASTNode {

  val wires: Seq[WireCppAST] = children flatMap {
    case w: WireCppAST => Seq(w)
    case b: BundleCppAST => b.wires
    case v: VecCppAST => v.wires
  } toIndexedSeq

  def sameTypeAs(other: CppASTNode): Boolean = {
    other match {
      case v: VecCppAST => children.head.sameTypeAs(v.children.head)
      case _ => false
    }
  }
}

class WireCppAST(val instanceName: String, val width: BigInt) extends CppASTLeaf {

  def sameTypeAs(other: CppASTNode): Boolean = {
    other match {
      case w: WireCppAST => true
      case _ => false
    }
  }
}
