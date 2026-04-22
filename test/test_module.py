# linux style shell only!
# open build.sh and set VENV path to your python venv path
# set exec permission: chmod +x build.sh
# run ./build (in the test directory)

if __name__ == "__main__":
    try:
        from shared_lib import Qpydict
    except Exception as err:
        print(f"Qypdict module import failed with error `{str(err)}`")
    else:
        print(f"Qypdict module import succeeded.\ncurrent version: {Qpydict.version()}")
