pkgname="xor-crypto"
epoch=2
pkgver=2
pkgrel=3
pkgdesc="xor encryptor program"
arch=("x86_64")
url="https://github.com/Imper927/xor_crypto"
license=('GPL')
depends=("libzip")
makedepends=("git" "cmake>=3.0")
source=("$pkgname::git://github.com/Imper927/xor_crypto.git")
md5sums=("SKIP")

prepare() {
	cd "$pkgname"
}

build() {
	cd "$pkgname"
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ .
	make
}

package() {
	cd "$pkgname"
	install -Dm755 ./xor_crypto "$pkgdir/usr/bin/$pkgname"
	echo "executing: $pkgname --action=install $pkgname ..."
	$pkgname --action=install $pkgname
}
