echo "Updating shaders ..."
export PATH=$PATH:/Users/mikaelsundell/Qt6.8/6.8.1/macos/bin
qsb color.vert --msl 20 -o color.vert.qsb
qsb color.frag --msl 20 -o color.frag.qsb