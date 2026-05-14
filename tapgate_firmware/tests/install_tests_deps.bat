@echo off
echo Updating pip...
python -m pip install --upgrade pip

echo Installing test dependencies...
pip install pyserial

echo Done.