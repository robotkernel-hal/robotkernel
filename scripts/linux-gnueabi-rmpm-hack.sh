export OECORE_ROOT=/home/schm_fl/data/local/oecore-i686

source $OECORE_ROOT/environment-setup-armv7a-vfp-neon-angstrom-linux-gnueabi

export PRU_BASE=/home/schm_fl/data/local/pru_base

export YAMLCPP_CFLAGS="-I/volume/software/common/foreign_packages/yaml-cpp/0.3.0/include/"
export YAMLCPP_LIBS="-L/volume/software/common/foreign_packages/yaml-cpp/0.3.0/lib/angstrom-armv7-gcc4.7 -lyaml-cpp"

#export LN_BASE="/volume/software/common/packages/links_and_nodes/0.2.8/"
#export LN_CFLAGS="-I${LN_BASE}/include"
#export LN_LIBS="-L${LN_BASE}/lib/arm-angstrom-linux-gnueabi -lln"

