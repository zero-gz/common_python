import argparse
import sys

parser = argparse.ArgumentParser(description = "input some integers:")
parser.add_argument("integers", type=int, nargs="+", help="an integer for accumulator")
parser.add_argument("--sum", action="store_const", const=sum, default=max, help="sum the integers(default: find the max)")

result = parser.parse_args(sys.argv[1:])
print result
print result.integers, result.sum
