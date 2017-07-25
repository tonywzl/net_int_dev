#! /bin/sh
NID_VERSION=$1
if [ -z ${NID_VERSION} ]
then
	NID_VERSION="1.2.3.4"
fi
cd ..
MYDIR=`pwd`
NID_SRC_DIR="${MYDIR}/src"
KERNMOD_SRC_DIR="${NID_SRC_DIR}/kernel/driver"
NIDS_SRC_DIR="${NID_SRC_DIR}/user/build/server"
ADMIN_SRC_DIR="${NID_SRC_DIR}/user/build/manager"
CLIENT_SRC_DIR="${NID_SRC_DIR}/user/build/agent"
MDS_SRC_DIR="${NID_SRC_DIR}/user/build/mds"
CONF="build/package.conf"
die()
{
	echo "***** ERROR : NID BUILD FAILURE *****"
	echo "$1"
	echo "ABORTING NID Build!"
	exit 1
}

echo;echo "***** Building NID binaries *****"
echo;echo "Building kernel module..."
cd "${KERNMOD_SRC_DIR}" || die "Unable to change to kernel mod directory ${KERNMOD_SRC_DIR}"
make clean
make || die "Unable to build kernel module"
cd -

echo;echo "Building server..."
cd "${NIDS_SRC_DIR}" || die "Unable to change to nid server src dir ${NIDS_SRC_DIR}"
make clean
make || die "Unable to build nid server binary"
cd -

echo;echo "Building admin binary..."
cd  "${ADMIN_SRC_DIR}" || die "Unable to change to nidadmin src directory ${ADMIN_SRC_DIR}"
make clean
make || die "Unable to build admin binary"
cd -

echo;echo "Building client binary..."
cd  "${CLIENT_SRC_DIR}" || die "Unable to change to nid client src directory ${CLIENT_SRC_DIR}"
make clean
make || die "Unable to build NID client binary"
cd -

echo;echo "Building mds binary..."
cd  "${MDS_SRC_DIR}" || die "Unable to change to nid mds src directory ${MDS_SRC_DIR}"
make clean
make || die "Unable to build NID mds binary"
cd -


{
        while read OP SRC TAR; do
                echo "operation: ${OP}"
                PWD=${PWD}
                echo "FROM: ${SRC} TO: ${TAR} "
                case ${OP} in
                d)
                        rm -rf ${SRC} || echo "Failed to delete ${SRC}"
                ;;
                md)
                        mkdir -p ${SRC} || echo "Failed to create ${SRC}"
                ;;
                c)
                        cp -f ${SRC} ${TAR} || echo "Failed to copy ${SRC} to ${TAR}"
                ;;
                cr)
                        cp -r ${SRC} ${TAR} || echo "Failed to copy ${SRC} to ${TAR}"
                ;;
                l)
			TAR_PATH=`dirname ${TAR}`
			echo "TAR_PATH ${TAR_PATH}"
			TAR_BASE=`basename ${TAR}`
			echo "TAR_BASE ${TAR_BASE}"
			cd ${TAR_PATH}
			CUR_DIR=`pwd`
			echo "CUR_DIR: ${CUR_DIR}"
                        ln -s ${SRC} ${TAR_BASE} || echo "Failed to link ${TAR} to ${SRC}"
			cd -
                ;;
                *)
                        echo "${OP} is invalid"
                esac
        done
} < ${CONF}
cp -a distro build
cd build/distro
zip --symlinks -r ../nid_${NID_VERSION}.zip ./* || echo "Failed to ZIP the amc distribution"
cd ..
cp -a DEBIAN distro
sed s/0.0.0.0/${NID_VERSION}/ DEBIAN/control > distro/DEBIAN/control
dpkg-deb --build distro nid_${NID_VERSION}_all.deb || die "Failed to deb the nid distribution"
cd ..
rm -rf distro

echo "Finish NID build"
exit 0
