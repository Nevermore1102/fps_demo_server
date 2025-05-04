FROM ubuntu:22.04

# 安装必要的工具和依赖
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libevent-dev \
    libevent-pthreads-2.1-7 \
    liblua5.3-dev \
    pkg-config \
    git \
    && rm -rf /var/lib/apt/lists/*

# 创建工作目录
WORKDIR /app

# 复制项目文件
COPY . .

# 构建项目
RUN mkdir build && cd build && \
    cmake .. && \
    make

# 暴露服务器端口
EXPOSE 8888

# 设置启动命令
CMD ["./build/game_server"] 