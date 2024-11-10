constexpr uint sScreenWidth = 1920u;
constexpr uint sScreenHeight = 1080u;
constexpr uint sHalfScreenWidth = sScreenWidth / 2;
constexpr uint sHalfScreenHeight = sScreenHeight / 2;

#ifdef DEBUG
#define LOCATION  __FILE__ << "   |   " << __LINE__ << '\n'
#define LOGMESSAGE(x)	std::cerr << "MESSAGE    |" << LOCATION << x << '\n' << std::endl
#define LOGWARNING(x)	std::cerr << "WARNING !  |" << LOCATION << x << '\n' << std::endl
#define LOGERROR(x)		std::cerr << "ERROR   !! |" << LOCATION << x << '\n' << std::endl

#else
#define LOGMESSAGE(x)
#define LOGWARNING(x)
#define LOGERROR(x)
#endif

#ifdef PLATFORM_LINUX
static inline const std::string sAssetsRoot = "../../../assets/";
static inline const std::string sDataRoot = sAssetsRoot + "data/";
static inline const std::string sDataRootWithoutAssetRoot = "data/";
#elif PLATFORM_WINDOWS
static inline const std::string sAssetsRoot = "assets/";
static inline const std::string sDataRoot = sAssetsRoot + "data/";
static inline const std::string sDataRootWithoutAssetRoot = "data/";

#endif // PLATFORM_LINUX

namespace Framework
{
	using EntityId = ushort;
	using MeshId = ushort;
}

namespace RTS
{
	enum class ArmyId : unsigned char { null, blue, red, player = blue, opponent = red };
	enum class CommandType : unsigned char { idle, moveTo, attack };
	enum class AggroLevel : unsigned char { holdFire, returnFire, pursuit };
}