import os
import subprocess
from subprocess import CalledProcessError
    
def setup():
    cmdForSubmodule = [
            "git",
            "submodule",
            "update",
            "--init",
            "--recursive"]
    _ = subprocess.run(
            cmdForSubmodule)
    try:
        _.check_returncode()
    except CalledProcessError:
        exit(-1)
    cwd = os.getcwd()
    parserDir = os.path.join(
            cwd, "pandaPIparser")
    cmdForMakingParser = [
            "make", "-C", 
            parserDir]
    _ = subprocess.run(
            cmdForMakingParser)
    try:
        _.check_returncode()
    except CalledProcessError:
        exit(-1)
    grounderDir = os.path.join(
            cwd,
            "pandaPIgrounder")
    cpddlDir = os.path.join(
            grounderDir,
            "cpddl")
    cmdForCpddl = [
            "make", 
            "-C",
            cpddlDir,
            "boruvka",
            "opts",
            "bliss",
            "lpsolve"]
    _ = subprocess.run(cmdForCpddl)
    try:
        _.check_returncode()
    except CalledProcessError:
        exit(-1)
    cmdForMakingCpddl = [
            "make",
            "-C",
            cpddlDir]
    _ = subprocess.run(
            cmdForMakingCpddl)
    try:
        _.check_returncode()
    except CalledProcessError:
        exit(-1)
    grounderMakefilePath = os.path.join(
            grounderDir,
            "src")
    cmdForMakingGrounder = [
            "make",
            "-C",
            grounderMakefilePath]
    _ = subprocess.run(
            cmdForMakingGrounder)
    try:
        _.check_returncode()
    except CalledProcessError:
        exit(-1)
    engineDir = os.path.join(
            cwd,
            "pandaPIengine")
    engineBuildDir = os.path.join(
            engineDir,
            "build")
    if not os.path.exists(engineBuildDir):
        os.mkdir(engineBuildDir)
    cmakeListsFile = os.path.join(
            cwd,
            "pandaPIengine",
            "src")
    cmdCMakeForEngine = [
            "cmake", 
            "-DCMAKE_BUILD_TYPE=Release",
            "-S",
            cmakeListsFile,
            "-D",
            engineBuildDir]
    _ = subprocess.run(cmdCMakeForEngine)
    try:
        _.check_returncode()
    except CalledProcessError:
        exit(-1)
    cmdForMakingEngine = [
            "make",
            "-C",
            engineBuildDir]
    _ = subprocess.run(cmdForMakingEngine)
    try:
        _.check_returncode()
    except CalledProcessError:
        exit(-1)

if __name__ == "__main__":
    setup()