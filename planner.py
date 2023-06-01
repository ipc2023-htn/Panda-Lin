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
        self._lilotane = os.path.join(
                cwd,
                "lilotane",
                "build",
                "lilotane")
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

    def __checkLilotaneResults(
            self, 
            proc : CompletedProcess) -> bool:
        outs = proc.stdout.split("\n")
        for i in range(len(outs) - 1, -1, -1):
            if "Unsolvable" in outs[i]:
                return False
        return True

    def __runLilotane(
            self,
            domainFile : str,
            taskFile : str) ->None:
        cmdSolving = [
                self._lilotane,
                domainFile,
                taskFile,
                "-cs",
                "-co=0"]
        proc = subprocess.run(
                cmdSolving,
                text=True,
                capture_output=True)
        return proc
    
    def __runPANDA(
            self,
            domainFile : str,
            taskFile : str,
            output : str,
            optimality : bool) -> CompletedProcess:
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
        if optimality:
            cmdSolving = [
                    engine,
                    "-s",
                    "--optimality",
                    groundingOutFile]
        else:
            cmdSolving = [
                    engine, 
                    "-s", 
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
        with open(output, "w") as o:
            started = False
            lines = proc.stdout.split("\n")
            for line in lines:
                if "Print Output Plan" in line:
                    started = True
                if started:
                    o.write(line + "\n")
        

    def plan(self, 
             domainFile : str, 
             taskFile : str,
             output : str,
             config : int) -> None:
        self.__runLinearizer(
                domainFile, taskFile)
        if config == 1:
            proc = self.__runLilotane(
                "domain-out.hddl", 
                "task-out.hddl")
            if not self.__checkLilotaneResults(proc):
                self.__runPANDA(domainFile, taskFile, output)
            else:
                with open(output, "w") as f:
                    f.write(proc.stdout)
        elif config == 2:
            proc = self.__runPANDA(
                "domain-out.hddl", 
                "task-out.hddl", False)
        elif config == 3:
            proc = self.__runPANDA("domain-out.hddl", "task-out.hddl", True)

if __name__ == "__main__":
    args = options.setup()
    planner = Planner()
    planner.plan(args.domain, 
                 args.task,
                 args.output,
                 args.config)