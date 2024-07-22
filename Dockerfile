# Utiliser une image de base Debian
FROM debian:latest

# Installer les dépendances nécessaires
RUN apt-get update && \
    apt-get install -y  build-essential  \
    libewf-dev libtsk-dev libfuse-dev git autotools-dev autoconf automake \
    pkg-config gettext autopoint libtool python3-libewf bison flex libssl-dev libsqlite3-dev zlib1g-dev ntpdate python3-tk \
    gcc g++ make mingw-w64 mingw-w64-tools mingw-w64-common wget git nano libgtk-3-dev

RUN apt-get install -y libafflib-dev libc3p0-java libvmdk-dev libvhdi-dev  libvhdi-dev libvmdk-dev libcppunit-dev libtool-bin

RUN git clone https://github.com/sleuthkit/sleuthkit.git && \
    cd sleuthkit   && \ 
    ./bootstrap && \
./configure mingw64 no-shared no-tests --prefix=/usr/x86_64-w64-mingw32 && \ 
  make && \
    make install 

RUN wget https://github.com/libyal/libewf/releases/download/20231119/libewf-experimental-20231119.tar.gz && \
tar -xvf libewf-experimental-20231119.tar.gz && \
 cd libewf-20231119 && \
 ./configure --host=x86_64-w64-mingw32 --prefix=/usr/x86_64-w64-mingw32 && \
 make && \
 make install 

 WORKDIR /app/data

#RUN gcc -o /app/input/list_fichier_x86 /app/input/main_x86.c -lewf -ltsk -lpthread

# Commande par défaut pour garder le conteneur en cours d'exécution
CMD ["tail", "-f", "/dev/null"]