# Copyright 2019-2021 Gentoo Authors
# Distributed under the terms of the GNU General Public License v2

EAPI=5

inherit eutils

DESCRIPTION="a stdio to tcp redirector daemon"
HOMEPAGE="https://github.com/rodan/iotcpd/"
SRC_URI="https://github.com/rodan/${PN}/archive/v${PV}.tar.gz -> ${P}.tar.gz"
S="${WORKDIR}/${P}/server"

LICENSE="GPL-3"
SLOT="0"
KEYWORDS="amd64 x86"
IUSE=""

DEPEND="x11-misc/makedepend"
RDEPEND=""

src_install() {
	doman "${S}/../doc/iotcpd.1"

	exeinto "usr/sbin"
	doexe iotcpd
}
