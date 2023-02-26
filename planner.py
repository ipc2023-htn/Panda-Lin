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

    def __runLinearizer(
            self,
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
            exit(-1)
    
    def __runHype(
            self,
            domainFile : str,
            taskFile : str) -> None:
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
        proc = subprocess.run(
                cmdSolving,
                text=True,
                capture_output=True)
        if proc.returncode:
            self.__log(proc)
            self.__runPANDA(
                    domainFile, taskFile)
            return
        with open("output", "w") as o:
            o.write(proc.stdout)
        
        
    
    def __runPANDA(
            self,
            domainFile : str,
            taskFile : str) -> CompletedProcess:
        cwd = os.getcwd()
        parser = os.path.join(
                cwd,
                "pandaPIparser",
                "pandaPIparser")
        parsingOutFile = os.path.join(
                cwd, "parsing-out")
        cmdParsing = [
                parser,
                domainFile,
                taskFile,
                parsingOutFile]
        proc = subprocess.run(
                cmdParsing,
                text=True,
                capture_output=True)
        try:
            proc.check_returncode()
        except subprocess.CalledProcessError:
            self.__log(proc)
            exit(-1)
        grounder = os.path.join(
                cwd,
                "pandaPIgrounder",
                "pandaPIgrounder")
        groundingOutFile = os.path.join(
                cwd,
                "grounding-out")
        cmdGrounding = [
                grounder,
                parsingOutFile,
                groundingOutFile]
        proc = subprocess.run(
                cmdGrounding,
                text=True,
                capture_output=True)
        try:
            proc.check_returncode()
        except subprocess.CalledProcessError:
            self.__log(proc)
            exit(-1) 
        engine = os.path.join(
                cwd,
                "pandaPIengine",
                "build",
                "pandaPIengine")
        cmdSolving = [
                engine,
                "-w",
                "2",
                groundingOutFile]
        proc = subprocess.run(
                cmdSolving,
                text=True,
                capture_output=True)
        try:
            proc.check_returncode()
        except subprocess.CalledProcessError:
            self.__log(proc)
            exit(-1)
        with open("output", "w") as o:
            o.write(proc.stdout)
        

    def plan(self, 
             domainFile : str, 
             taskFile : str) -> None:
        self.__runLinearizer(
                domainFile, taskFile)
        self.__runHype(
                domainFile, taskFile)

if __name__ == "__main__":
    args = options.setup()
    planner = Planner()
    planner.plan(args.domain, 
                 args.task)