//------------------------------------------------------------------------------
/*
    This file is part of ripple-libpp: https://github.com/ripple/ripple-libpp
    Copyright (c) 2016 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <ripple/protocol/AccountID.h>
#include <ripple/protocol/digest.h>
#include <ripple/protocol/HashPrefix.h>
#include <ripple/protocol/JsonFields.h>
#include <ripple/protocol/PublicKey.h>
#include <ripple/protocol/SecretKey.h>
#include <ripple/protocol/Sign.h>
#include <ripple/protocol/st.h>
#include <ripple/protocol/TxFlags.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/json/to_string.h>
#include <ripple/json/json_reader.h>
#include <ripple-libpp/version.hpp>
#include <boost/version.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <algorithm>
#include <sstream>


std::string serialize(ripple::STTx const& tx)
{
    using namespace ripple;

    return strHex(tx.getSerializer().peekData());
}

std::shared_ptr<ripple::STTx const> deserialize(std::string blob)
{
    using namespace ripple;

    auto ret{ strUnHex(blob) };

    if (!ret.second || !ret.first.size())
        Throw<std::runtime_error>("transaction not valid hex");

    SerialIter sitTrans{ makeSlice(ret.first) };
    // Can Throw
    return std::make_shared<STTx const>(std::ref(sitTrans));
}

int main (int argc, char** argv)
{
#if defined(__GNUC__) && !defined(__clang__)
    auto constexpr gccver = (__GNUC__ * 100 * 100) +
                            (__GNUC_MINOR__ * 100) +
                            __GNUC_PATCHLEVEL__;

    static_assert (gccver >= 50100,
        "GCC version 5.1.0 or later is required to compile rippled.");
#endif

    static_assert (BOOST_VERSION >= 105700,
        "Boost version 1.57 or later is required to compile rippled");

    using namespace ripple;
    Json::Value result;
    if (argc < 2)
    {
        result["status"] = "error";
        result["error"] = "Command(key_gen/key_conv/key_chk/tx_sign) is required.";
        std::cout << result.toStyledString() << std::endl;
        return 0;
    }
    
    std::string cmd = argv[1];
    if (cmd == "key_gen")
    {
        // 1.random seed
        using namespace ripple;
        Seed seed = ripple::randomSeed();
        PublicKey pk;
        SecretKey sk;
        std::tie (pk, sk) = generateKeyPair (KeyType::secp256k1, seed);
        AccountID account = calcAccountID (pk);

        result["status"] = "success";
        result["public"] = to_string(account);
        result["private"] = toBase58(seed);
    }
    else if (cmd == "key_conv" && argc == 3)
    {
        auto const seed = parseGenericSeed(argv[2]);
        PublicKey pk;
        SecretKey sk;
        std::tie (pk, sk) = generateKeyPair (KeyType::secp256k1, *seed);
        AccountID account = calcAccountID (pk);

        result["status"] = "success";
        result["public"] = to_string(account);
    }
    else if (cmd == "key_chk" && argc == 3)
    {
        auto const account = parseBase58<AccountID>(argv[2]);
        if(account == boost::none)
        {
            result["status"] = "success";
            result["result"] = "false";
        }
        else
        {
            result["status"] = "success";
            result["result"] = "true";
        }
    }
    else if (cmd == "tx_sign")
    {
        if (argc != 4)
        {
            result["status"] = "error";
            result["msg"] = "Format:tx_sign secretKey tx";
        }
        else
        {
            auto const seed = parseGenericSeed(argv[2]);
            PublicKey pk;
            SecretKey sk;
            std::tie (pk, sk) = generateKeyPair (KeyType::secp256k1, *seed);
            AccountID account = calcAccountID (pk);
            
            std::string str_tx = argv[3];
            //auto const buf = sign(pk, sk, makeSlice(str_tx));
            //std::cout << buf << std::endl;
            Json::Value jv;
            Json::Reader jr;
            if (!jr.parse(str_tx, jv))
            {
                std::cout << jr.getFormatedErrorMessages() << "\n" << std::endl;
            }

            auto const destination = parseBase58<AccountID>(
                jv["Destination"].asString());
            STTx tx(ttPAYMENT,
                [&](auto& obj)
            {
                // General transaction fields
                obj[sfAccount] = account;
                if (jv.isMember("Fee"))
                {
                    obj[sfFee] = STAmount{ uint64_t(jv["Fee"].asInt()) };
                }
                else
                {
                    obj[sfFee] = STAmount{ 1000 };
                }
                
                obj[sfFlags] = tfFullyCanonicalSig;
                if (jv.isMember("Sequence"))
                {
                    obj[sfSequence] = jv["Sequence"].asInt();
                }
                obj[sfSigningPubKey] = pk.slice();
                // Payment-specific fields
                
                STAmount amount;
                if (jv["Amount"].isMember("currency"))
                {
                    auto const gateway1 = parseBase58<AccountID>(
                        jv["Amount"]["issuer"].asString());
                    amount = STAmount(
                        Issue(to_currency(jv["Amount"]["currency"].asString()), *gateway1),
                        jv["Amount"]["value"].asInt());
                }
                else
                {
                    amount = STAmount (jv["Amount"].asDouble());    // VRP
                }
                
                obj[sfAmount] = amount;
                obj[sfDestination] = *destination;
                if (jv.isMember("SendMax"))
                {
                    auto const gateway2 = parseBase58<AccountID>(
                        jv["SendMax"]["issuer"].asString());
                    obj[sfSendMax] = STAmount(
                        Issue(to_currency(jv["SendMax"]["currency"].asString()), *gateway2),
                        jv["SendMax"]["value"].asInt());
                }
                
            });
            tx.sign(pk, sk);
    
            auto const serialized = serialize(tx);
            //std::cout << "\nAfter signing: \n" <<
            //    tx.getJson(0).toStyledString() << "\n" <<
            //    "Serialized: " << serialized << "\n";
            result["hash"] = tx.getJson(0)["hash"].asString();
            result["tx_blob"] = serialized;
            result["status"] = "success";
        }
    }
    else
    {
        std::cout << "Command:" << cmd << "argc:" << argc << "\n" << std::endl;
        std::string msg = "Command:" + cmd + " not found.";
        result["error"] = msg;
        result["status"] = "error";
    }
    std::cout << result.toStyledString() << std::endl;
    return 1;
}

