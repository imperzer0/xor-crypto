pkgname="xor-crypto"
epoch=3
pkgver=8
pkgrel=1
pkgdesc="xor encryptor program"
arch=("x86_64")
url="https://github.com/imperzer0/xor-crypto"
license=('GPL')
depends=("gcc-libs" "glibc" "libzip")
makedepends=("cmake>=3.0" "gcc" "fish-completions>=1:8-4" "parse-arguments>=1:8-1")
source=("local://main.cpp" "local://CMakeLists.txt" "local://functions.hpp" "local://archive_and_encrypt.hpp" "local://completions.hpp" "local://terminal_output.hpp")
md5sums=("SKIP" "SKIP" "SKIP" "SKIP" "SKIP" "SKIP")
install=xor-crypto.install

build()
{
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ .
	make
}

package()
{
	install -Dm755 "./$pkgname" "$pkgdir/usr/bin/$pkgname"
}
