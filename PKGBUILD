pkgname="xor-crypto"
epoch=3
pkgver=4
pkgrel=3
pkgdesc="xor encryptor program"
arch=("x86_64")
url="https://github.com/Imper927/xor_crypto"
license=('GPL')
depends=("libzip")
makedepends=("cmake>=3.0")
source=("local://main.cpp" "local://CMakeLists.txt" "local://archive_and_encrypt.hpp" "local://completions.hpp" "local://terminal_output.hpp")
md5sums=("SKIP" "SKIP" "SKIP" "SKIP" "SKIP")
install=xor-crypto.install

build()
{
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ .
	make
}

package()
{
	install -Dm755 ./xor_crypto "$pkgdir/usr/bin/$pkgname"
}
