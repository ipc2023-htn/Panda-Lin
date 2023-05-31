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
    linearizerDir = os.path.join(
            cwd, "linearizer")
    cmdForMakingLinearizer = [
            "make",
            "-C",
            linearizerDir]
    _ = subprocess.run(
            cmdForMakingLinearizer)
    try:
        _.check_returncode()
    except CalledProcessError:
        exit(-1)
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
            "-B",
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
    lilotaneDir = os.path.join(
            cwd, "lilotane")
    lilotaneBuildDir = os.path.join(
            lilotaneDir, "build")
    if not os.path.exists(lilotaneBuildDir):
        os.mkdir(lilotaneBuildDir)
    cmdCMakeLilotane = [
            "cmake",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DIPASIRSOLVER=glucose4",
            "-S", lilotaneDir,
            "-B", lilotaneBuildDir]
    _ = subprocess.run(cmdCMakeLilotane)
    try:
        _.check_returncode()
    except CalledProcessError:
        exit(-1)
    cmdMakingLilotant = [
            "make", "-C",
            lilotaneBuildDir]
    _ = subprocess.run(cmdMakingLilotant)
    try:
        _.check_returncode()
    except CalledProcessError:
        exit(-1)

if __name__ == "__main__":
    setup()