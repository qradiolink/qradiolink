#!/bin/sh

#  _                   _          _      _     _                          _
# | |_ _ __ __ ___   _(_)___   __| | ___| |__ (_) __ _ _ __    _ __   ___| |_
# | __| '__/ _` \ \ / / / __| / _` |/ _ \ '_ \| |/ _` | '_ \  | '_ \ / _ \ __|
# | |_| | | (_| |\ V /| \__ \| (_| |  __/ |_) | | (_| | | | |_| | | |  __/ |_
#  \__|_|  \__,_| \_/ |_|___(_)__,_|\___|_.__/|_|\__,_|_| |_(_)_| |_|\___|\__|
#
#
#               Documentation: <http://travis.debian.net>


## Copyright ##################################################################
#
# Copyright © 2015, 2016, 2017, 2018 Chris Lamb <lamby@debian.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

## Functions ##################################################################

set -eu

Info () {
	echo "I: ${*}" >&2
}

Error () {
	echo "E: ${*}" >&2
}

Indent () {
	sed -e 's@^@  @g' "${@}"
}

if [ "${TRAVIS_BUILD_NUMBER:-}" = "" ]
then
	Error "TRAVIS_BUILD_NUMBER not set; are you running under TravisCI?"
	exit 2
fi

## Configuration ##############################################################

if [ -f debian/changelog ]
then
	SOURCE="$(dpkg-parsechangelog | awk '/^Source:/ { print $2 }')"
	VERSION="$(dpkg-parsechangelog | awk '/^Version:/ { print $2 }')"
else
	# Fallback to parsing debian/control if debian/changelog does not exist
	SOURCE="$(awk '/^Source:/ { print $2 }' debian/control)"
	VERSION="0"
fi

if [ "${SOURCE}" = "" ] || [ "${VERSION}" = "" ]
then
	Error "Could not determine source and version from packaging"
	exit 2
fi

Info "Starting build of ${SOURCE} using travis.debian.net"

TAG="travis.debian.net/${SOURCE}"
TRAVIS_DEBIAN_LINTIAN="${TRAVIS_DEBIAN_LINTIAN:-true}"
TRAVIS_DEBIAN_BUILD_DIR="${TRAVIS_DEBIAN_BUILD_DIR:-/build}"
TRAVIS_DEBIAN_TARGET_DIR="${TRAVIS_DEBIAN_TARGET_DIR:-../}"
TRAVIS_DEBIAN_NETWORK_ENABLED="${TRAVIS_DEBIAN_NETWORK_ENABLED:-false}"
TRAVIS_DEBIAN_INCREMENT_VERSION_NUMBER="${TRAVIS_DEBIAN_INCREMENT_VERSION_NUMBER:-false}"

#### Distribution #############################################################

TRAVIS_DEBIAN_BACKPORTS="${TRAVIS_DEBIAN_BACKPORTS:-}" # list
TRAVIS_DEBIAN_EXPERIMENTAL="${TRAVIS_DEBIAN_EXPERIMENTAL:-false}"

if [ "${TRAVIS_DEBIAN_DISTRIBUTION:-}" = "" ]
then
	Info "Automatically detecting distribution"

	TRAVIS_DEBIAN_DISTRIBUTION="${TRAVIS_BRANCH:-}"

	# Populate from branch name directly if we are not running under Travis.
	if [ "${TRAVIS_DEBIAN_DISTRIBUTION:-}" = "" ]
	then
		TRAVIS_DEBIAN_DISTRIBUTION="$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo master)"
	fi

	# Strip leading "debian/"
	TRAVIS_DEBIAN_DISTRIBUTION="${TRAVIS_DEBIAN_DISTRIBUTION##debian/}"

	# Detect backports
	case "${TRAVIS_DEBIAN_DISTRIBUTION}" in
		*-backports)
			TRAVIS_DEBIAN_BACKPORTS="${TRAVIS_DEBIAN_DISTRIBUTION}"
			TRAVIS_DEBIAN_DISTRIBUTION="${TRAVIS_DEBIAN_DISTRIBUTION%%-backports}"
			;;
		*-backports-sloppy)
			TRAVIS_DEBIAN_DISTRIBUTION="${TRAVIS_DEBIAN_DISTRIBUTION%%-backports-sloppy}"
			TRAVIS_DEBIAN_BACKPORTS="${TRAVIS_DEBIAN_DISTRIBUTION}-backports ${TRAVIS_DEBIAN_DISTRIBUTION}-backports-sloppy"
			;;
		backports/*)
			TRAVIS_DEBIAN_BACKPORTS="${TRAVIS_DEBIAN_DISTRIBUTION##backports/}-backports"
			TRAVIS_DEBIAN_DISTRIBUTION="${TRAVIS_DEBIAN_DISTRIBUTION##backports/}"
			;;
		backports-sloppy/*)
			TRAVIS_DEBIAN_DISTRIBUTION="${TRAVIS_DEBIAN_DISTRIBUTION##backports-sloppy/}"
			TRAVIS_DEBIAN_BACKPORTS="${TRAVIS_DEBIAN_DISTRIBUTION}-backports ${TRAVIS_DEBIAN_DISTRIBUTION}-backports-sloppy"
			;;
		*_bpo7+*|*_bpo70+*)
			TRAVIS_DEBIAN_BACKPORTS="wheezy-backports"
			TRAVIS_DEBIAN_DISTRIBUTION="wheezy"
			;;
		*_bpo8+*)
			TRAVIS_DEBIAN_BACKPORTS="jessie-backports"
			TRAVIS_DEBIAN_DISTRIBUTION="jessie"
			;;
		*_bpo9+*)
			TRAVIS_DEBIAN_BACKPORTS="stretch-backports"
			TRAVIS_DEBIAN_DISTRIBUTION="stretch"
			;;
		*_bpo10+*)
			TRAVIS_DEBIAN_BACKPORTS="buster-backports"
			TRAVIS_DEBIAN_DISTRIBUTION="buster"
			;;
	esac
fi

# Detect/rewrite codenames
case "${TRAVIS_DEBIAN_DISTRIBUTION}" in
	oldoldstable)
		TRAVIS_DEBIAN_DISTRIBUTION="wheezy"
		;;
	oldstable)
		TRAVIS_DEBIAN_DISTRIBUTION="jessie"
		;;
	stable)
		TRAVIS_DEBIAN_DISTRIBUTION="stretch"
		;;
	testing)
		TRAVIS_DEBIAN_DISTRIBUTION="buster"
		;;
	unstable|master|debian)
		TRAVIS_DEBIAN_DISTRIBUTION="sid"
		;;
	experimental)
		TRAVIS_DEBIAN_DISTRIBUTION="sid"
		TRAVIS_DEBIAN_EXPERIMENTAL="true"
		;;
esac

# Detect derivatives
if [ "${TRAVIS_DEBIAN_DERIVATIVE:-}" = "" ]
then
	case "${TRAVIS_DEBIAN_DISTRIBUTION}" in
		precise*|trusty*|xenial*|zesty*|artful*|bionic*)
			TRAVIS_DEBIAN_DERIVATIVE="ubuntu"
			;;
		*)
			TRAVIS_DEBIAN_DERIVATIVE="debian"
			;;
	esac
fi

# TRAVIS_DEBIAN_DISTRIBUTION and TRAVIS_DEBIAN_DERIVATIVE are now set, so set dependent options

case "${TRAVIS_DEBIAN_DISTRIBUTION}" in
	wheezy)
		TRAVIS_DEBIAN_GIT_BUILDPACKAGE="${TRAVIS_DEBIAN_GIT_BUILDPACKAGE:-git-buildpackage}"
		TRAVIS_DEBIAN_GIT_BUILDPACKAGE_OPTIONS="${TRAVIS_DEBIAN_GIT_BUILDPACKAGE_OPTIONS:-}"
		;;
	*)
		TRAVIS_DEBIAN_GIT_BUILDPACKAGE="${TRAVIS_DEBIAN_GIT_BUILDPACKAGE:-gbp buildpackage}"
		TRAVIS_DEBIAN_GIT_BUILDPACKAGE_OPTIONS="${TRAVIS_DEBIAN_GIT_BUILDPACKAGE_OPTIONS:---git-submodules}"
		;;
esac

case "${TRAVIS_DEBIAN_DISTRIBUTION}" in
	wheezy|jessie)
		TRAVIS_DEBIAN_AUTOPKGTEST_RUN="${TRAVIS_DEBIAN_AUTOPKGTEST_RUN:-adt-run}"
		TRAVIS_DEBIAN_AUTOPKGTEST_SEPARATOR="${TRAVIS_DEBIAN_AUTOPKGTEST_SEPARATOR:----}"
		;;
	*)
		TRAVIS_DEBIAN_AUTOPKGTEST_RUN="${TRAVIS_DEBIAN_AUTOPKGTEST_RUN:-autopkgtest}"
		TRAVIS_DEBIAN_AUTOPKGTEST_SEPARATOR="${TRAVIS_DEBIAN_AUTOPKGTEST_SEPARATOR:---}"
		;;
esac

case "${TRAVIS_DEBIAN_DISTRIBUTION}" in
	sid)
		TRAVIS_DEBIAN_SECURITY_UPDATES="${TRAVIS_DEBIAN_SECURITY_UPDATES:-false}"
		;;
	*)
		TRAVIS_DEBIAN_SECURITY_UPDATES="${TRAVIS_DEBIAN_SECURITY_UPDATES:-true}"
		;;
esac

if [ "${TRAVIS_DEBIAN_MIRROR:-}" = "" ]
then
	case "${TRAVIS_DEBIAN_DERIVATIVE}" in
		ubuntu)
			TRAVIS_DEBIAN_MIRROR="http://archive.ubuntu.com/ubuntu"
			;;
		*)
			TRAVIS_DEBIAN_MIRROR="http://deb.debian.org/debian"
			;;
	esac
fi

if [ "${TRAVIS_DEBIAN_COMPONENTS:-}" = "" ]
then
	case "${TRAVIS_DEBIAN_DERIVATIVE}" in
		ubuntu)
			TRAVIS_DEBIAN_COMPONENTS="main restricted universe multiverse"
			;;
		*)
			TRAVIS_DEBIAN_COMPONENTS="main"
			;;
	esac
fi


## Detect autopkgtest tests ###################################################

if [ -e "debian/tests/control" ] || grep -qs -E '^(XS-)?Testsuite: autopkgtest' debian/control
then
	TRAVIS_DEBIAN_AUTOPKGTEST="${TRAVIS_DEBIAN_AUTOPKGTEST:-true}"
else
	TRAVIS_DEBIAN_AUTOPKGTEST="${TRAVIS_DEBIAN_AUTOPKGTEST:-false}"
fi

## Print configuration ########################################################

Info "Building on: ${TRAVIS_DEBIAN_DERIVATIVE}"
Info "Using distribution: ${TRAVIS_DEBIAN_DISTRIBUTION}"
Info "Saving to Docker tag: ${TAG}"
Info "With components: ${TRAVIS_DEBIAN_COMPONENTS}"
Info "Backports enabled: ${TRAVIS_DEBIAN_BACKPORTS:-<none>}"
Info "Experimental enabled: ${TRAVIS_DEBIAN_EXPERIMENTAL}"
Info "Security updates enabled: ${TRAVIS_DEBIAN_SECURITY_UPDATES}"
Info "Will use extra repository: ${TRAVIS_DEBIAN_EXTRA_REPOSITORY:-<not set>}"
Info "Extra repository's key URL: ${TRAVIS_DEBIAN_EXTRA_REPOSITORY_GPG_URL:-<not set>}"
Info "Will build under: ${TRAVIS_DEBIAN_BUILD_DIR}"
Info "Will store results under: ${TRAVIS_DEBIAN_TARGET_DIR}"
Info "Using mirror: ${TRAVIS_DEBIAN_MIRROR}"
Info "Network enabled during build: ${TRAVIS_DEBIAN_NETWORK_ENABLED}"
Info "Run Lintian after build: ${TRAVIS_DEBIAN_LINTIAN}"
Info "Builder command: ${TRAVIS_DEBIAN_GIT_BUILDPACKAGE}"
Info "Builder command options: ${TRAVIS_DEBIAN_GIT_BUILDPACKAGE_OPTIONS}"
Info "Increment version number: ${TRAVIS_DEBIAN_INCREMENT_VERSION_NUMBER}"
Info "Run autopkgtests after build: ${TRAVIS_DEBIAN_AUTOPKGTEST}"
Info "DEB_BUILD_OPTIONS: ${DEB_BUILD_OPTIONS:-<not set>}"
Info "DEB_BUILD_PROFILES: ${DEB_BUILD_PROFILES:-<not set>}"

## Increment version number ###################################################

if [ "${TRAVIS_DEBIAN_INCREMENT_VERSION_NUMBER}" = true ] || [ ! -f debian/changelog ]
then
	cat >debian/changelog.new <<EOF
${SOURCE} (${VERSION}+travis${TRAVIS_BUILD_NUMBER}) UNRELEASED; urgency=medium

  * Automatic build.

 -- travis.debian.net <nobody@nobody.example>  $(date --utc -R)

EOF
	# Don't rely on debian/changelog existing
	touch debian/changelog
	cat <debian/changelog >>debian/changelog.new
	mv debian/changelog.new debian/changelog
	# Force addition of changelog despite .gitignore
	git add -f debian/changelog
	git commit -m "Incrementing version number."
fi

## Git setup #################################################################

git config remote.origin.fetch '+refs/heads/*:refs/remotes/origin/*'
git fetch

for X in $(git branch -r | grep -v HEAD)
do
	git branch --track "$(echo "${X}" | sed -e 's@.*/@@g')" "${X}" || true
done

## Build ######################################################################

cat >Dockerfile <<EOF
FROM ${TRAVIS_DEBIAN_DERIVATIVE}:${TRAVIS_DEBIAN_DISTRIBUTION}
RUN echo "deb ${TRAVIS_DEBIAN_MIRROR} ${TRAVIS_DEBIAN_DISTRIBUTION} ${TRAVIS_DEBIAN_COMPONENTS}" > /etc/apt/sources.list
RUN echo "deb-src ${TRAVIS_DEBIAN_MIRROR} ${TRAVIS_DEBIAN_DISTRIBUTION} ${TRAVIS_DEBIAN_COMPONENTS}" >> /etc/apt/sources.list
EOF

for X in ${TRAVIS_DEBIAN_BACKPORTS}
do
	cat >>Dockerfile <<EOF
RUN echo "deb ${TRAVIS_DEBIAN_MIRROR} ${X} ${TRAVIS_DEBIAN_COMPONENTS}" >> /etc/apt/sources.list
RUN echo "deb-src ${TRAVIS_DEBIAN_MIRROR} ${X} ${TRAVIS_DEBIAN_COMPONENTS}" >> /etc/apt/sources.list
EOF
done

if [ "${TRAVIS_DEBIAN_SECURITY_UPDATES}" = true ]
then
    case "${TRAVIS_DEBIAN_DERIVATIVE}" in
	ubuntu)
	    cat >>Dockerfile <<EOF
RUN echo "deb http://archive.ubuntu.com/ubuntu/ ${TRAVIS_DEBIAN_DISTRIBUTION}-updates ${TRAVIS_DEBIAN_COMPONENTS}" >> /etc/apt/sources.list
RUN echo "deb http://archive.ubuntu.com/ubuntu/ ${TRAVIS_DEBIAN_DISTRIBUTION}-security ${TRAVIS_DEBIAN_COMPONENTS}" >> /etc/apt/sources.list
EOF
	    ;;
	*)
	    cat >>Dockerfile <<EOF
RUN echo "deb http://security.debian.org/ ${TRAVIS_DEBIAN_DISTRIBUTION}/updates ${TRAVIS_DEBIAN_COMPONENTS}" >> /etc/apt/sources.list
RUN echo "deb-src http://security.debian.org/ ${TRAVIS_DEBIAN_DISTRIBUTION}/updates ${TRAVIS_DEBIAN_COMPONENTS}" >> /etc/apt/sources.list
EOF
	    ;;
    esac
fi

if [ "${TRAVIS_DEBIAN_EXPERIMENTAL}" = true ]
then
	cat >>Dockerfile <<EOF
RUN echo "deb ${TRAVIS_DEBIAN_MIRROR} experimental ${TRAVIS_DEBIAN_COMPONENTS}" >> /etc/apt/sources.list
RUN echo "deb-src ${TRAVIS_DEBIAN_MIRROR} experimental ${TRAVIS_DEBIAN_COMPONENTS}" >> /etc/apt/sources.list
EOF
fi

TRAVIS_DEBIAN_EXTRA_PACKAGES=""

case "${TRAVIS_DEBIAN_EXTRA_REPOSITORY:-}" in
	https:*)
		TRAVIS_DEBIAN_EXTRA_PACKAGES="${TRAVIS_DEBIAN_EXTRA_PACKAGES} apt-transport-https"
		;;
esac

if [ "${TRAVIS_DEBIAN_EXTRA_REPOSITORY_GPG_URL:-}" != "" ]
then
	TRAVIS_DEBIAN_EXTRA_PACKAGES="${TRAVIS_DEBIAN_EXTRA_PACKAGES} wget gnupg"
fi

if [ "${TRAVIS_DEBIAN_LINTIAN}" = "true" ]
then
	TRAVIS_DEBIAN_EXTRA_PACKAGES="${TRAVIS_DEBIAN_EXTRA_PACKAGES} lintian"
fi

for X in ${TRAVIS_DEBIAN_BACKPORTS}
do
	cat >>Dockerfile <<EOF
RUN echo "Package: *" >> /etc/apt/preferences.d/travis_debian_net
RUN echo "Pin: release a=${X}" >> /etc/apt/preferences.d/travis_debian_net
RUN echo "Pin-Priority: 500" >> /etc/apt/preferences.d/travis_debian_net
EOF
done

cat >>Dockerfile <<EOF
RUN echo force-unsafe-io > /etc/dpkg/dpkg.cfg.d/force-unsafe-io
RUN echo 'Acquire::EnableSrvRecords "false";' > /etc/apt/apt.conf.d/90srvrecords
RUN apt-get update && apt-get dist-upgrade --yes
EOF

if [ "${TRAVIS_DEBIAN_AUTOPKGTEST}" = "true" ]
then
	# Take a slight diversion to make a minimal environment for
	# autopkgtests
	cp Dockerfile Dockerfile.autopkgtests
	cat >>Dockerfile.autopkgtests <<EOF
WORKDIR $(pwd)
COPY . .

RUN rm -f Dockerfile Dockerfile.autopkgtests
RUN mkdir -p ${TRAVIS_DEBIAN_BUILD_DIR}
EOF

	Info "Using Dockerfile:"
	Indent Dockerfile.autopkgtests

	Info "Building Docker image ${TAG}.autopkgtests"
	docker build --tag="${TAG}.autopkgtests" --file Dockerfile.autopkgtests .

	rm -f Dockerfile.autopkgtests
fi

cat >>Dockerfile <<EOF
RUN DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends build-essential equivs devscripts git-buildpackage ca-certificates pristine-tar ${TRAVIS_DEBIAN_EXTRA_PACKAGES}

WORKDIR $(pwd)
COPY . .
EOF

if [ "${TRAVIS_DEBIAN_EXTRA_REPOSITORY_GPG_URL:-}" != "" ]
then
	cat >>Dockerfile <<EOF
RUN wget -O- "${TRAVIS_DEBIAN_EXTRA_REPOSITORY_GPG_URL}" | apt-key add -
EOF
fi

# We're adding the extra repository only after the essential tools have been
# installed, so that we have apt-transport-https if the repository needs it.
if [ "${TRAVIS_DEBIAN_EXTRA_REPOSITORY:-}" != "" ]
then
	cat >>Dockerfile <<EOF
RUN echo "deb ${TRAVIS_DEBIAN_EXTRA_REPOSITORY}" >> /etc/apt/sources.list
RUN echo "deb-src ${TRAVIS_DEBIAN_EXTRA_REPOSITORY}" >> /etc/apt/sources.list
RUN apt-get update
EOF
fi

cat >>Dockerfile <<EOF
RUN env DEBIAN_FRONTEND=noninteractive DEB_BUILD_PROFILES="${DEB_BUILD_PROFILES:-}" mk-build-deps --install --remove --tool 'apt-get -o Debug::pkgProblemResolver=yes --no-install-recommends --yes' debian/control

RUN rm -f Dockerfile
RUN git checkout Dockerfile .travis.yml || true
RUN mkdir -p ${TRAVIS_DEBIAN_BUILD_DIR}

CMD ${TRAVIS_DEBIAN_GIT_BUILDPACKAGE} ${TRAVIS_DEBIAN_GIT_BUILDPACKAGE_OPTIONS} --git-ignore-branch --git-export-dir=${TRAVIS_DEBIAN_BUILD_DIR} -uc -us -sa
EOF

Info "Using Dockerfile:"
Indent Dockerfile

Info "Building Docker image ${TAG}"
docker build --tag="${TAG}" .

Info "Restoring Dockerfile to previous state (safe to ignore errors)"
rm -f Dockerfile
git checkout -- Dockerfile || true

CIDFILE="$(mktemp --dry-run)"
ARGS="--cidfile=${CIDFILE}"

if [ "${TRAVIS_DEBIAN_NETWORK_ENABLED}" != "true" ]
then
	ARGS="${ARGS} --net=none"
fi

Info "Running build"
# shellcheck disable=SC2086
docker run --privileged --env=DEB_BUILD_OPTIONS="${DEB_BUILD_OPTIONS:-}" --env=DEB_BUILD_PROFILES="${DEB_BUILD_PROFILES:-}" ${ARGS} "${TAG}"

Info "Copying build artefacts to ${TRAVIS_DEBIAN_TARGET_DIR}"
mkdir -p "${TRAVIS_DEBIAN_TARGET_DIR}"
docker cp "$(cat "${CIDFILE}")":"${TRAVIS_DEBIAN_BUILD_DIR}"/ - \
	| tar xf - -C "${TRAVIS_DEBIAN_TARGET_DIR}" --strip-components=1

if [ "${TRAVIS_DEBIAN_AUTOPKGTEST}" = "true" ]
then
	Info "Running autopkgtests"

	docker run --privileged --env TRAVIS=true --volume "$(readlink -f "${TRAVIS_DEBIAN_TARGET_DIR}"):${TRAVIS_DEBIAN_BUILD_DIR}" --interactive "${TAG}.autopkgtests" /bin/sh - <<EOF
set -eu

cat <<EOS >/usr/sbin/policy-rc.d
#!/bin/sh
exit 0
EOS
chmod a+x /usr/sbin/policy-rc.d
apt-get install --yes --no-install-recommends autopkgtest autodep8

TEST_RET="0"
${TRAVIS_DEBIAN_AUTOPKGTEST_RUN} ${TRAVIS_DEBIAN_BUILD_DIR}/*.changes ${TRAVIS_DEBIAN_AUTOPKGTEST_SEPARATOR} null || TEST_RET="\${?}"
echo "I: ${TRAVIS_DEBIAN_AUTOPKGTEST_RUN} exited with status code \${TEST_RET}" >&2
if [ "\${TEST_RET}" != "0" ] && [ "\${TEST_RET}" != "2" ]
then
	exit \${TEST_RET}
fi
EOF
fi

Info "Build successful"

Info "$(basename "${TRAVIS_DEBIAN_TARGET_DIR}"/*.changes)"
docker start "$(cat "${CIDFILE}")" >/dev/null
Indent "${TRAVIS_DEBIAN_TARGET_DIR}"/*.changes

if ls "${TRAVIS_DEBIAN_TARGET_DIR}"/*.buildinfo >/dev/null 2>&1
then
	Info "$(basename "${TRAVIS_DEBIAN_TARGET_DIR}"/*.buildinfo)"
	Indent "${TRAVIS_DEBIAN_TARGET_DIR}"/*.buildinfo
fi

DEBC="$(mktemp)"
docker exec "$(cat "${CIDFILE}")" /bin/sh -c "debc ${TRAVIS_DEBIAN_BUILD_DIR}/*.changes" > "${DEBC}"
if [ "$(wc -l < "${DEBC}")" -lt 500 ]
then
	Info "debc ${TRAVIS_DEBIAN_BUILD_DIR}/*.changes"
	Indent "${DEBC}"
fi
rm -f "${DEBC}"

Info "Removing container"
docker rm -f "$(cat "${CIDFILE}")" >/dev/null
rm -f "${CIDFILE}"

#  _                   _          _      _     _                          _
# | |_ _ __ __ ___   _(_)___   __| | ___| |__ (_) __ _ _ __    _ __   ___| |_
# | __| '__/ _` \ \ / / / __| / _` |/ _ \ '_ \| |/ _` | '_ \  | '_ \ / _ \ __|
# | |_| | | (_| |\ V /| \__ \| (_| |  __/ |_) | | (_| | | | |_| | | |  __/ |_
#  \__|_|  \__,_| \_/ |_|___(_)__,_|\___|_.__/|_|\__,_|_| |_(_)_| |_|\___|\__|
#
