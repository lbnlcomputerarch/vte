// adapted from chisel3.iotesters.Driver by Albert Chen

package vte

import chisel3._
import logger.Logger

import scala.reflect.ClassTag
import scala.util.DynamicVariable

object VDriver {

  private val optionsManagerVar = new DynamicVariable[Option[TesterOptionsManager]](None)
  def optionsManager = optionsManagerVar.value.getOrElse(new TesterOptionsManager)

  /**
    * This verilates the device under test and generates the necessary C++
    * testbench files with an optionsManager to control all the options of the
    * toolchain components
    *
    * @param dutGenerator    The device under test, a subclass of a Chisel3 module
    * @param optionsManager  Use this to control options like which backend to use
    * @return                Returns true if all tests in testerGen pass
    */
  def execute[T <: Module](
                            dutGenerator: () => T,
                            optionsManager: TesterOptionsManager
                          )(implicit tag: ClassTag[T]): Boolean = {
    optionsManagerVar.withValue(Some(optionsManager)) {
      Logger.makeScope(optionsManager) {
        if (optionsManager.topName.isEmpty) {
          if (optionsManager.targetDirName == ".") {
            optionsManager.setTargetDirName("test_run_dir")
          }
          val genClassName = tag.runtimeClass.getName
          val testerName = genClassName.split("""\$\$""").headOption.getOrElse("") + genClassName.hashCode.abs
          optionsManager.setTargetDirName(s"${optionsManager.targetDirName}/$testerName")
        }

        val dut = setupVerilatorBackend(dutGenerator, optionsManager)
        true
      }
    }
  }

  def apply[T <: Module](dutGen: () => T,
                                   testbenchCppFileName: String = "")(implicit tag: ClassTag[T]): Boolean = {

    val optionsManager = new TesterOptionsManager {
      testerOptions = testerOptions.copy(testbenchCppFile = testbenchCppFileName)
    }

    execute(dutGen, optionsManager)
  }
}
