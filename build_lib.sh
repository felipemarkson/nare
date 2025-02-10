set -e
VERSION=0.0.2

PROJECT_DIR=$(pwd)

rm -rf dist

# DEBUG
gcc -O0 -ggdb -rdynamic -fPIC -shared -o objs/libnare.so.$VERSION -I includes/ src/nare.c
gcc -O0 -ggdb -fPIC  -o objs/nare.o -I includes/ -c src/nare.c
ar rcs objs/libnare.a.$VERSION objs/nare.o
mkdir -p dist/nare-$VERSION-gnu-amd64-DEBUG/lib
mkdir -p dist/nare-$VERSION-gnu-amd64-DEBUG/include

mv objs/libnare.so.$VERSION dist/nare-$VERSION-gnu-amd64-DEBUG/lib/
mv objs/libnare.a.$VERSION  dist/nare-$VERSION-gnu-amd64-DEBUG/lib/
cd dist/nare-$VERSION-gnu-amd64-DEBUG/lib/
ln -s libnare.so.$VERSION  libnare.so
ln -s libnare.a.$VERSION   libnare.a
cd $PROJECT_DIR

cp includes/nare.h dist/nare-$VERSION-gnu-amd64-DEBUG/include
cp LICENSE dist/nare-$VERSION-gnu-amd64-DEBUG/
cp LICENSE-CC0 dist/nare-$VERSION-gnu-amd64-DEBUG/
cp README.md dist/nare-$VERSION-gnu-amd64-DEBUG/

tar -czf dist/nare-$VERSION-gnu-amd64-DEBUG.tar.gz dist/nare-$VERSION-gnu-amd64-DEBUG/

# RELEASE
gcc -O2 -g0 -rdynamic -fPIC -shared -o objs/libnare.so.$VERSION -I includes/ src/nare.c
gcc -O2 -g0 -fPIC -o objs/nare.o -I includes/ -c src/nare.c
ar rcs objs/libnare.a.$VERSION objs/nare.o
mkdir -p dist/nare-$VERSION-gnu-amd64/lib
mkdir -p dist/nare-$VERSION-gnu-amd64/include

mv objs/libnare.so.$VERSION dist/nare-$VERSION-gnu-amd64/lib/
mv objs/libnare.a.$VERSION  dist/nare-$VERSION-gnu-amd64/lib/
cd dist/nare-$VERSION-gnu-amd64/lib/
ln -s libnare.so.$VERSION  libnare.so
ln -s libnare.a.$VERSION   libnare.a
cd $PROJECT_DIR

cp includes/nare.h dist/nare-$VERSION-gnu-amd64/include
cp LICENSE dist/nare-$VERSION-gnu-amd64/
cp LICENSE-CC0 dist/nare-$VERSION-gnu-amd64/
cp README.md dist/nare-$VERSION-gnu-amd64/

tar -czf dist/nare-$VERSION-gnu-amd64.tar.gz dist/nare-$VERSION-gnu-amd64/