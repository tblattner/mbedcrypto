#include <catch.hpp>

#include "mbedcrypto/ecp.hpp"
#include "generator.hpp"

#include <iostream>
///////////////////////////////////////////////////////////////////////////////
namespace {
using namespace mbedcrypto;
///////////////////////////////////////////////////////////////////////////////

std::ostream&
operator<<(std::ostream& s, const pk::action_flags& f) {
    auto bs = [](bool b) {
        return b ? "true" : "false";
    };

    s << "encrypt: " << bs(f.encrypt) << " , "
      << "decrypt: " << bs(f.decrypt) << " , "
      << "sign: "    << bs(f.sign)    << " , "
      << "verify: "  << bs(f.verify);
    return  s;
}

///////////////////////////////////////////////////////////////////////////////
} // namespace anon
///////////////////////////////////////////////////////////////////////////////
TEST_CASE("ec type checks", "[types][pk]") {
    using namespace mbedcrypto;

    if ( supports(pk_t::eckey)  ||  supports(pk_t::eckey_dh) ) {
        ecp my_key; // default as eckey
        REQUIRE( my_key.can_do(pk_t::eckey)) ;
        REQUIRE( !my_key.has_private_key() ); // no key is provided
        REQUIRE( test::icompare(my_key.name(), "EC") );
        REQUIRE( my_key.can_do(pk_t::eckey) );
        REQUIRE( my_key.can_do(pk_t::eckey_dh) );
        if ( supports(pk_t::ecdsa) ) {
            REQUIRE( my_key.can_do(pk_t::ecdsa) );
        } else {
            REQUIRE( !my_key.can_do(pk_t::ecdsa) );
        }

        auto af = my_key.what_can_do();
        // my_key has no key. all capabilities must be false
        REQUIRE_FALSE( (af.encrypt || af.decrypt || af.sign || af.verify) );

        my_key.reset_as(pk_t::eckey_dh);
        REQUIRE( test::icompare(my_key.name(), "EC_DH") );
        REQUIRE( !my_key.has_private_key() );
        REQUIRE( my_key.can_do(pk_t::eckey_dh) );
        REQUIRE( my_key.can_do(pk_t::eckey) );
        REQUIRE( !my_key.can_do(pk_t::ecdsa) ); // in any circumstances
        // my_key has no key. all capabilities must be false
        REQUIRE_FALSE( (af.encrypt || af.decrypt || af.sign || af.verify) );
    }

    if ( supports(pk_t::ecdsa) ) {
        ecp my_key(pk_t::ecdsa);
        REQUIRE( test::icompare(my_key.name(), "ECDSA") );
        REQUIRE( !my_key.has_private_key() );
        REQUIRE( my_key.can_do(pk_t::ecdsa) );
        REQUIRE( !my_key.can_do(pk_t::eckey) );
        REQUIRE( !my_key.can_do(pk_t::eckey_dh) );
        auto af = my_key.what_can_do();
        // my_key has no key. all capabilities must be false
        REQUIRE_FALSE( (af.encrypt || af.decrypt || af.sign || af.verify) );
    }
}

TEST_CASE("ec key tests", "[pk]") {
    using namespace mbedcrypto;

    if ( supports(features::pk_export)  &&  supports(pk_t::eckey) ) {
        ecp gen;
        REQUIRE_THROWS( gen.generate_key(curve_t::none) );

        const std::initializer_list<curve_t> Items = {
            curve_t::secp192r1,
            curve_t::secp224r1,
            curve_t::secp256r1,
            curve_t::secp384r1,
            curve_t::secp521r1,
            curve_t::secp192k1,
            curve_t::secp224k1,
            curve_t::secp256k1,
            curve_t::bp256r1,
            curve_t::bp384r1,
            curve_t::bp512r1,
            //curve_t::curve25519, // reported bug in mbedtls!
        };

        auto key_test = [&gen](curve_t ctype, const auto& afs) {
            gen.generate_key(ctype);
            auto pri_data = gen.export_key(pk::pem_format);
            auto pub_data = gen.export_public_key(pk::pem_format);

            ecp pri;
            pri.import_key(pri_data);
            REQUIRE( pri.type() == gen.type() );
            REQUIRE( (pub_data == pri.export_public_key(pk::pem_format)) );

            ecp pub;
            pub.import_public_key(pub_data);
            REQUIRE( pub.type() == gen.type() );

            REQUIRE( check_pair(pub, pri) );

            REQUIRE( (pri.what_can_do() == std::get<0>(afs)) );
            REQUIRE( (pub.what_can_do() == std::get<1>(afs)) );
        };

        auto eckey_afs = []() {
            if ( supports(pk_t::ecdsa) ) {
                return std::make_tuple(
                        pk::action_flags{false, false, true, true},
                        pk::action_flags{false, false, false, true}
                        );
            } else {
                return std::make_tuple(
                        pk::action_flags{false, false, false, false},
                        pk::action_flags{false, false, false, false}
                        );
            }
        };

        for ( auto i : Items ) {
            REQUIRE_NOTHROW( key_test(i, eckey_afs()) );
        }
    }
}
