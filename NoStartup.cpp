#define __$OnceI(l, c) __I__once##c##__##l
#define __$Once(l, c) __$OnceI(l, c)
#define $Once static ::dhc::Once __$Once(__LINE__, __COUNTER__) = []()

namespace dhc {
  struct Once {
    template <typename Fn>
    constexpr Once(Fn&& fn) {
      (void) static_cast<Fn>(fn)();
    }
  };

  [[gnu::noinline]]
  int Gen(int i) {
    return i + 1;
  }
} // namespace dhc

static constinit int X = 1;
static constinit int Y = 1;
static constinit int Z = 1;

$Once { ::X = 2; };
$Once { ::Y = 4; };
$Once { ::Z = 8; };

int main(int V, char**) {
  if (V == 0) return 55;
  return X + Y + Z; // Returns 14!!
}
