#! /bin/bash

MYDIR=`pwd`
NID_SRC_DIR="${MYDIR}/src"
KERNMOD_SRC_DIR="${NID_SRC_DIR}/kernel/nbd"
NIDS_SRC_DIR="${NID_SRC_DIR}/user/server"
ADMIN_SRC_DIR="${NID_SRC_DIR}/user/admin"
CLIENT_SRC_DIR="${NID_SRC_DIR}/user/nbd"
DISTDIR="${MYDIR}/DISTDIR"

die()
{
	echo "***** ERROR : NID BUILD FAILURE *****"
	echo "$1"
	echo "ABORTING NID Build!"
	exit 1
}

echo;echo "Cleaning up..."
mkdir -p "${DISTDIR}"
rm -vf "${DISTDIR}/*"

echo;echo "***** Building NID binaries *****"
echo;echo "Building kernel module..."
pushd "${KERNMOD_SRC_DIR}" || die "Unable to change to kernel mod directory ${KERNMOD_SRC_DIR}"
 make clean
 make || die "Unable to build kernel module"
 echo "   copying kernel module to distrib directory"
popd
 cp -v "${KERNMOD_SRC_DIR}/nbd.ko" "${DISTDIR}"

 echo;echo "Building server..."
pushd "${NIDS_SRC_DIR}" || die "Unable to change to nid server src dir ${NIDS_SRC_DIR}"
 make clean
 make || die "Unable to build nid server binary"
popd
 echo "    copying server to build distrib directory"
 cp -v "${NIDS_SRC_DIR}/nids" "${DISTDIR}"

 echo;echo "Building admin binary..."
pushd "${ADMIN_SRC_DIR}" || die "Unable to change to nidadmin src directory ${ADMIN_SRC_DIR}"
 make clean
 make || die "Unable to build admin binary"
popd
 echo "    copying admin binary to distrib directory"
 cp -v "${ADMIN_SRC_DIR}/nidadm" "${DISTDIR}"

 echo;echo "Building client binary..."
pushd "${CLIENT_SRC_DIR}" || die "Unable to change to nid client src directory ${CLIENT_SRC_DIR}"
 make clean
 make || die "Unable to build NID client binary"
popd
 echo "    copying client binary to distrib directory"
 cp -v "${CLIENT_SRC_DIR}/nbd-client" "${DISTDIR}"

echo;echo "Cleaning up object files from build, if possible..."
pushd "${KERNMOD_SRC_DIR}" && make clean && popd
pushd "${NIDS_SRC_DIR}" && make clean && popd
pushd "${ADMIN_SRC_DIR}" && make clean && popd
pushd "${CLIENT_SRC_DIR}" && make clean && popd

exit 0
