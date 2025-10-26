from stream import RWStream
from test import assert_eq
import time


class Xv6:
    def __init__(self, stream: RWStream):
        self.stream = stream

    def run(self, program: str):
        time.sleep(0.1)
        self.stream.writeline(program)
        assert_eq(self.stream.readline(), program)
        assert_eq(self.stream.readline(), "")
