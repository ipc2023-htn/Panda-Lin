import argparse

def setup():
    argparser = argparse.ArgumentParser(
            "Options for the Planner")
    argparser.add_argument(
            "--domain", required=True,
            help="path to the domain file")
    argparser.add_argument(
            "--task", required=True,
            help="path to the task file")
    argparser.add_argument(
            "--output", required=True,
            help="path to the output plan file")
    argparser.add_argument(
            "--config", required=True, 
            help="configuration to be used")
    return argparser.parse_args()