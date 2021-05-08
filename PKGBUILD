pkgname="xor-crypto"
epoch=2
pkgver=4
pkgrel=1
pkgdesc="xor encryptor program"
arch=("x86_64")
url="https://github.com/Imper927/xor_crypto"
license=('GPL')
depends=("libzip")
makedepends=("cmake>=3.0")
source=("local://main.cpp" "local://CMakeLists.txt")
md5sums=("SKIP" "SKIP")

build() {
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ .
	make
}

package() {
	install -Dm755 ./xor_crypto "$pkgdir/usr/bin/$pkgname"
	echo -e "\033[35mexecuting: $pkgname --action=install $pkgname ...\033[0m"
	sudo $pkgname --action=install $pkgname
}
