import os
import logging
import subprocess
import options
from subprocess import CompletedProcess


class Planner:
    def __init__(self) -> None:
        cwd = os.getcwd()
        self._linearizer = os.path.join(
                cwd,
                "linearizer",
                "linearizer")
        self._hype = os.path.join(
                cwd,
                "HyperTensioN",
                "Hype.rb")
        if os.path.exists("error.log"):
            subprocess.run(["rm", "error.log"])
        logFormat = "{asctime:s}\n- {message:s}"
        logging.basicConfig(
                filename="error.log",
                format=logFormat,
                style="{")
        self._logger = logging.getLogger(__name__)
    
    def __log(self, 
              proc : CompletedProcess) -> None:
        msg = (proc.stdout + "\n- "
               + proc.stderr + "\n- ")
        self._logger.error(
                msg, exc_info=True)

    def plan(self, 
             domainFile : str, 
             taskFile : str) -> None:
        cmdLinearizing = [
                self._linearizer,
                domainFile,
                taskFile,
                "domain-out.hddl",
                "task-out.hddl"]
        proc = subprocess.run(
                cmdLinearizing,
                text=True, 
                capture_output=True)
        try:
            proc.check_returncode()
        except subprocess.CalledProcessError:
            self.__log(proc)
        hypeExtensions = [
                "dejavu",
                "typredicate",
                "pullup",
                "run"]
        cmdSolving = [
                "ruby",
                self._hype,
                "domain-out.hddl",
                "task-out.hddl"]
        cmdSolving += hypeExtensions
        proc = subprocess.run(cmdSolving,
                              text=True,
                              capture_output=True)
        try:
            proc.check_returncode()
        except subprocess.CalledProcessError:
            self.__log(proc)
            return
        with open("output", "w") as o:
            o.write(proc.stdout)

if __name__ == "__main__":
    args = options.setup()
    planner = Planner()
    planner.plan(args.domain, 
                 args.task)