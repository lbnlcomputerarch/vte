// written by Albert Chen

package vte

import chisel3.HasChiselExecutionOptions
import firrtl.{ComposableOptions, ExecutionOptionsManager, HasFirrtlOptions}

case class TesterOptions(testbenchCppFile: String = "") extends ComposableOptions

trait HasTesterOptions {
  self: ExecutionOptionsManager =>

  var testerOptions = TesterOptions()

  parser.note("tester options")

  parser.opt[String]("testbench-cpp-file-name")
    .abbr("ttcf")
    .foreach { x => testerOptions = testerOptions.copy(testbenchCppFile = x) }
    .text("file containing implementation of testbench::run() method")
}

class TesterOptionsManager
  extends ExecutionOptionsManager("chisel-testers")
    with HasTesterOptions
    with HasChiselExecutionOptions
    with HasFirrtlOptions{
}

