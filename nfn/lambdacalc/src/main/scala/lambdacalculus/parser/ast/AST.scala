package lambdacalculus.parser.ast

import scala.util.parsing.input.Positional


object BinaryOp extends Enumeration {
  type BinaryOp = Value
  val Add = Value("ADD")
  val Sub = Value("SUB")
  val Mult = Value("MULT")
  val Div = Value("DIV")
  val Eq = Value("EQ")
  val Neq = Value("NEQ")
  val Lt = Value("LT")
  val Gt = Value("GT")
  val Lte = Value("LTE")
  val Gte = Value("GTE")
}

object UnaryOp extends Enumeration {
  type UnaryOp = Value
  val Cdr = Value("CDR")
}


object LambdaDSLTest extends App {
  import LambdaDSL._

  val a: Expr = 'x @: "y" @: (('x * 1) - "y")

  val b: Call = "/WordCount" call ("/doc/doc1" :: Nil)

}

object LambdaDSL {
//  implicit def call(symbolNameAndExprs: (Symbol, List[Expr])): Call = symbolAndExprsToCall(symbolNameAndExprs)
  implicit def symbolAndExprsToCall(symbolNameAndExprs: (Symbol, List[Expr])): Call = Call(symbolNameAndExprs._1.name, symbolNameAndExprs._2)
  implicit def stringAndExprsToCall(nameAndExprs: (String, List[Expr])): Call = Call(nameAndExprs._1, nameAndExprs._2)
  implicit def symbolToExpr (sym: Symbol): Variable = Variable.toVar(sym)
  implicit def stringToExpr (str: String): Variable = Variable(str)
  implicit def intToConstant(c: Int):Constant  = Constant(c)
}

sealed abstract class Expr extends Positional {
  def @: (symbolName: Symbol): Expr = Clos(symbolName.name,this)
  def @: (stringName: String): Expr = Clos(stringName,this)
  def ! (arg: Expr) = Application(this,arg)
  def + (expr: Expr) = BinaryExpr(BinaryOp.Add, this, expr)
  def - (expr: Expr) = BinaryExpr(BinaryOp.Sub, this, expr)
  def * (expr: Expr) = BinaryExpr(BinaryOp.Mult, this, expr)
  def / (expr: Expr) = BinaryExpr(BinaryOp.Div, this, expr)
}

case class Clos(boundVariable: String, body: Expr) extends Expr {
  override def equals(o: Any) = o match {
    case that: Clos => that.body == body
    case _ => false
  }
  override def hashCode = body.hashCode
}
case class Application(rator: Expr, rand: Expr) extends Expr

object Variable {
  implicit def toVar (sym: Symbol): Variable = Variable(sym.name)
}

case class Variable(name: String, accessValue: Int = -1) extends Expr {
  def apply(args: List[Expr]): Call = call(args)
  def call(args: List[Expr]): Call = {
    import LambdaDSL._
    Symbol(this.name) -> args
  }
}

object Constant {
}

case class Constant(i: Int) extends Expr

case class UnaryExpr(op: UnaryOp.UnaryOp, v: Expr) extends Expr
case class BinaryExpr(op: BinaryOp.BinaryOp, v1: Expr, v2:Expr) extends Expr
case class Let(name: String, letExpr: Expr, code: Option[Expr]) extends Expr
case class IfElse(test: Expr, thenn: Expr, otherwise: Expr) extends Expr
case class NopExpr() extends Expr
case class Call(name: String, args: List[Expr]) extends Expr




//object LambdaNFNPreprocessor {
//  private def replaceLets(letName: String, letExpr: Expr, expr: Expr): Expr = {
//    def rpl(expr: Expr): Expr = {
//      expr match {
//        case clos@Clos(arg, body) =>
//          // Only replace the name if the closure does not introduce the same name
////          if (arg != letName)
//            Clos(arg, rpl(expr))
////          else clos
//        case Application(fun, arg) => Application(rpl(fun), rpl(arg))
//        case UnaryExpr(op, v) => UnaryExpr(op, rpl(v))
//        case BinaryExpr(op, v1, v2) =>  BinaryExpr(op, rpl(v1), rpl(v2))
//        case v @ Variable(name, _) => if(name == letName) letExpr else v
//        case c @ Constant(i) => c
//        case Let(name, expr, maybeCodeExpr) => Let(name, rpl(expr), maybeCodeExpr map { rpl })
//        case IfElse(test, thenn, otherwise) => IfElse(rpl(test), rpl(thenn), rpl(otherwise))
//        case Call(name, args) =>
//          if(name == letName)
//            Call(name, args map { rpl })
//          else
//            Call(name, args map { rpl })
//
//        case n @ NopExpr() => n
//        case _ => throw new NotImplementedError(s"LambdaPrettyPrinter cannot pretty print: $expr")
//      }
//    }
//    rpl(expr)
//  }
//
//
//  // Recursively goes througha all expressions and for each Let expression tries to replace all occurrences in the subtree
//  def apply(expr: Expr): Expr = expr match {
//    case let @ Let(name, letExpr, maybeCode) => {
//      maybeCode match {
//        case Some(code) => replaceLets(name, letExpr, code)
//        case None => let
//      }
//    }
//    case Clos(arg, body) => Clos(arg, apply(body))
//    case Application(fun, arg) => Application(apply(fun), apply(arg))
//    case UnaryExpr(op, v) => UnaryExpr(op, apply(v))
//    case BinaryExpr(op, v1, v2) =>  BinaryExpr(op, apply(v1), apply(v2))
//    case v:Variable => v
//    case c:Constant => c
//    case IfElse(test, thenn, otherwise) => IfElse(apply(test), apply(thenn), apply(otherwise))
//    case Call(name, args) => Call(name, args map { apply })
//    case n @ NopExpr() => n
//    case _ => expr
//  }
//}



