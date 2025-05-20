#pragma once

#include <cstdint>
#include <string_view>

namespace lox::utils {

enum class exit_codes : int32_t {
	ok,
	usage = 64,  /* command line usage error */
	dataerr,     /* data format error */
	noinput,     /* cannot open input */
	nouser,      /* addressee unknown */
	nohost,      /* host name unknown */
	unavailable, /* service unavailable */
	software,    /* internal software error */
	oserr,       /* system error (e.g., can't fork) */
	osfile,      /* critical OS file missing */
	cantcreat,   /* can't create (user) output file */
	ioerr,       /* input/output error */
	tempfail,    /* temp failure; user is invited to retry */
	protocol,    /* remote error in protocol */
	noperm,      /* permission denied */
	config,      /* configuration error */
};

[[nodiscard]] constexpr auto as_int(const exit_codes code) noexcept -> int32_t {
	return static_cast<int32_t>(code);
}

[[nodiscard]] constexpr auto exit_codes_name(const exit_codes code) noexcept -> std::string_view {
	using namespace std::string_view_literals;
	switch (code) {
		using enum exit_codes;

		case ok:          return "ok"sv;
		case usage:       return "command line usage error"sv;
		case dataerr:     return "data format error"sv;
		case noinput:     return "cannot open input"sv;
		case nouser:      return "addressee unknown"sv;
		case nohost:      return "host name unknown"sv;
		case unavailable: return "service unavailable"sv;
		case software:    return "internal software error"sv;
		case oserr:       return "system error (e.g., can't fork)"sv;
		case osfile:      return "critical OS file missing"sv;
		case cantcreat:   return "can't create (user) output file"sv;
		case ioerr:       return "input/output error"sv;
		case tempfail:    return "temp failure; user is invited to retry"sv;
		case protocol:    return "remote error in protocol"sv;
		case noperm:      return "permission denied"sv;
		case config:      return "configuration error"sv;
		default: break;
	}
	return "unknown error"sv;
}

} // namespace lox::utils
