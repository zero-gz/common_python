import logging

logger = logging.getLogger("a.test")
logger.setLevel(logging.DEBUG)

ch = logging.FileHandler("test.txt")
console = logging.StreamHandler()

formater = logging.Formatter("%(asctime)s -- %(name)s: %(message)s")
ch.setFormatter(formater)
console.setFormatter(formater)

logger.addHandler(ch)
logger.addHandler(console)

logger.info("just a test %s", "123")

logger1 = logging.getLogger("b.test.m")
logger1.addHandler(console)
logger1.setLevel(logging.DEBUG)

logger1.debug("just a logger 1 test")