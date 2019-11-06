## Run Tests with setuptools

python3 setup.py test

## Build and Install Locally

python3 -m pip install --user --verbose .

## Build sdist and upload to PyPI-Test

python3 setup.py sdist
python3 -m twine upload --repository-url https://test.pypi.org/legacy/ dist/*

## Build wheels and upload to PyPI-Test

docker pull quay.io/pypa/manylinux2010_x86_64
docker run --rm -it -v /home/tb/cobs:/src quay.io/pypa/manylinux2010_x86_64

and run
/src/misc/python-wheels/manylinux_x86_64.sh

python3 -m twine upload --repository-url https://test.pypi.org/legacy/ dist/*

## Test distributed inside a virtualenv

virtualenv a
cd a
source bin/activate

python3 -m pip install --index-url https://test.pypi.org/simple/ --no-deps cobs_index
