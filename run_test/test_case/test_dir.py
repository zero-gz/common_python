import os

if __name__ == "__main__":
    print "get path:", os.path.join(os.path.dirname(os.path.realpath(__file__)), 'conan.profile')