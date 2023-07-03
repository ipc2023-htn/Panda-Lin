import os
import logging
import subprocess
import options
from subprocess import CompletedProcess


class Planner:
    def __init__(self, cache : str) -> None:
        cwd = os.getcwd()
        self._linearizer = os.path.join(
                os.path.dirname(__file__),
                "linearizer",
                "linearizer")
        self._lilotane = os.path.join(
                os.path.dirname(__file__),
                "lilotane",
                "build",
                "lilotane")
        self.__cache = cache

    def __runLinearizer(
            self,
            domainFile : str,
            taskFile : str) -> None:
        outDomain = os.path.join(
                self.__cache, "domain-out.hddl")
        outTask = os.path.join(
                self.__cache, "task-out.hddl")
        cmdLinearizing = [
                self._linearizer,
                domainFile,
                taskFile,
                outDomain,
                outTask]
        proc = subprocess.run(
                cmdLinearizing,
                text=True, 
                capture_output=True)
        try:
            proc.check_returncode()
        except subprocess.CalledProcessError:
            exit(-1)
        return (outDomain, outTask)

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
        parser = os.path.join(
                os.path.dirname(__file__),
                "pandaPIparser",
                "pandaPIparser")
        parsingOutFile = os.path.join(
                self.__cache, "parsing-out")
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
            exit(-1)
        grounder = os.path.join(
                os.path.dirname(__file__),
                "pandaPIgrounder",
                "pandaPIgrounder")
        groundingOutFile = os.path.join(
                self.__cache,
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
            exit(-1) 
        engine = os.path.join(
                os.path.dirname(__file__),
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
            exit(-1)
        with open(output, "w") as o:
            started = False
            lines = proc.stdout.split("\n")
            for line in lines:
                if "==>" in line:
                    started = True
                if started:
                    o.write(line + "\n")
        

    def plan(self, 
             domainFile : str, 
             taskFile : str,
             output : str,
             config : int) -> None:
        domain, task = self.__runLinearizer(
                domainFile, taskFile)
        if config == 1:
            proc = self.__runLilotane(
                domain, 
                task)
            if not self.__checkLilotaneResults(proc):
                self.__runPANDA(domainFile, taskFile, output, False)
            else:
                with open(output, "w") as f:
                    started = False
                    lines = proc.stdout.split("\n")
                    for line in lines:
                        if "==>" in line:
                            started = True
                        if started:
                            f.write(line + "\n")
        elif config == 2:
            proc = self.__runPANDA(
                domain, 
                task, output, False)
        elif config == 3:
            proc = self.__runPANDA(domain, task, output, True)

if __name__ == "__main__":
    args = options.setup()
    planner = Planner(args.cache)
    planner.plan(args.domain, 
                 args.task,
                 args.output,
                 args.config)