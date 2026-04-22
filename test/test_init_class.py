#!/bin/python

if __name__ == "__main__":
    try:
        from shared_lib import Qpydict

        cls = Qpydict.QPyDict((5, 4));
    except Exception as err:
        print(f"Qypdict module import failed with error `{str(err)}`")
    else:
        print(f"Qypdict module import succeeded.\n")
