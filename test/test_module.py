# build cmd: python setup.py build_ext --build-lib test/shared_lib --build-temp test/build_temp on the main dir

if __name__ == "__main__":
    try:
        from shared_lib import Qpydict
    except Exception as err:
        print(f"Qypdict module import failed with error `{str(err)}`")
    else:
        print(f"Qypdict module import succeeded.\ncurrent version: {Qpydict.version()}")
