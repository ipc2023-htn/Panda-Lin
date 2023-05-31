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
    return argparser.parse_args()