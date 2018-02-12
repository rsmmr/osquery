require File.expand_path("../Abstract/abstract-osquery-formula", __FILE__)

class Broker < AbstractOsqueryFormula
  desc "Broker Communication Library"
  homepage "https://github.com/bro/broker"
  url "https://github.com/bro/broker.git", # Need git url for recursive clone
      :branch => "topic/actor-system"
      #:revision => "68a36ed81480ba935268bcaf7b6f2249d23436da"
	  #:tag => "v0.6"
  head "https://github.com/bro/broker.git"
  version "0.6"
  revision 4

  needs :cxx11

  bottle do
      root_url "https://osquery-packages.s3.amazonaws.com/bottles"
      cellar :any_skip_relocation
  end

  depends_on "caf"
  depends_on "openssl"
  depends_on "cmake" => :build

  # Use static libcaf
  patch :DATA

  def install
    prepend "CXXFLAGS", "-std=c++11 -Wextra -Wall"
    args = %W[--prefix=#{prefix} --disable-python --enable-static-only --with-caf=#{default_prefix}]

    system "./configure", *args
    system "make"
    system "make", "install"
  end

end

__END__
