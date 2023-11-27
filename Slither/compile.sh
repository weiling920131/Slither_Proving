rm /etc/apt/sources.list.d/cuda.list
rm /etc/apt/sources.list.d/nvidia-ml.list
apt-key del 7fa2af80
apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/3bf863cc.pub
apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/machine-learning/repos/ubuntu1804/x86_64/7fa2af80.pub   


apt update
pip install tensorboard 
pip install zstandard
pip install psutil
apt install clang-format -y
apt-get install tmux vim screen -y


python3 -m pip uninstall protobuf -y
python3 -m pip install --user protobuf==3.20.1
