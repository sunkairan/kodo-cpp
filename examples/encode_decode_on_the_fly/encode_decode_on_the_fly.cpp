// Copyright Steinwurf ApS 2011-2012.
// Distributed under the "STEINWURF RESEARCH LICENSE 1.0".
// See accompanying file LICENSE.rst or
// http://www.steinwurf.com/licensing
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

#include <kodocpp/kodocpp.hpp>


int main(void)
{
    // Set the number of symbols (i.e. the generation size in RLNC
    // terminology) and the size of a symbol in bytes
    uint32_t max_symbols = 42;
    uint32_t max_symbol_size = 160;

    bool trace_enabled = true;

    //Initilization of encoder and decoder
    kodocpp::encoder_factory encoder_factory(
        kodocpp::code_type::on_the_fly,
        kodocpp::finite_field::binary8,
        max_symbols,
        max_symbol_size,
        trace_enabled);

    kodocpp::encoder encoder = encoder_factory.build();

    kodocpp::decoder_factory decoder_factory(
        kodocpp::code_type::on_the_fly,
        kodocpp::finite_field::binary8,
        max_symbols,
        max_symbol_size,
        trace_enabled);

    kodocpp::decoder decoder = decoder_factory.build();

    // Allocate some storage for a "payload" the payload is what we would
    // eventually send over a network
    std::vector<uint8_t> payload(encoder.payload_size());

    // Allocate some data to encode. In this case we make a buffer
    // with the same size as the encoder's block size (the max.
    // amount a single encoder can encode)
    std::vector<uint8_t> data_in(encoder.block_size());
    std::vector<uint8_t> data_out(encoder.block_size());

    // Just for fun - fill the data with random data
    std::generate(data_in.begin(), data_in.end(), rand);

    // Keeps track of which symbols have been decoded
    std::vector<bool> decoded(max_symbols, false);

    // We are starting the encoding / decoding looop without having
    // added any data to the encoder - we will add symbols on-the-fly
    while(!decoder.is_complete())
    {
        uint32_t bytes_used;

        // Randomly choose to add a new symbol (with 50% porbability)
        // if the encoder rank is less than the maximum number of symbols
        if((rand() % 2) && encoder.rank() < encoder.symbols())
        {
            // The rank of an encoder  indicates how many symbols have been added,
            // i.e. how many symbols are available for encoding
            uint32_t rank = encoder.rank();

            // Calculate the offset to the nex symbol to insert
            uint8_t* symbol = data_in.data() + (rank * encoder.symbol_size());

            encoder.set_symbol(rank,
                               symbol,
                               encoder.symbol_size());
        }

        bytes_used = encoder.encode(payload.data());

        std::cout << "Payload generated by encoder, rank = " << encoder.rank()
                  << ", bytes used = " << bytes_used << "\n";

        if(rand() % 2)
        {
            std::cout << "packet dropped\n";
            continue;
        }

        // Packet got through - pass that packet to the decoder
        decoder.decode(payload.data());

        // The rank of the decoder indicates how many symbols have been
        // decoded or partially decoded
        std::cout << "Payload processed by the decoder, current rank = "
                  << decoder.rank() << "\n";

        // Check the decoder wehter it is partially complet
        // For on-the-fly decoding the decoder has to support partial
        // decoding tracker

        if(decoder.has_partial_decoding_tracker() &&
           decoder.is_partial_complete())
        {
            for(uint32_t i = 0; i < decoder.symbols(); ++i)
            {
                if(!decoder.is_symbol_uncoded(i))
                {
                    continue;
                }

                if(!decoded[i])
                {
                    // Update that this symbol now has been decoded
                    std::cout << "Symbol " << i << " was decoded\n";
                    decoded[i] = true;

                    uint32_t offset = i * encoder.symbol_size();
                    uint8_t* target = data_out.data() + offset;

                    // Verify the decoded symbol

                    // Copy out the individual symbol from the decoder
                    decoder.copy_symbol(i, target,
                                        encoder.symbol_size());

                    // Verify the symbol against the original data
                    auto start = data_out.begin() + offset;
                    auto end = start + encoder.symbol_size();
                    if(std::equal(start, end, data_out.begin() + offset))
                    {
                        std::cout << "Symbol " << i << " decoded correctly.\n";
                    }
                    else
                    {
                        std::cout << "SYMBOL " << i << "DECODING FAILED.\n";
                    }
                }
            }
        }
    }

    decoder.copy_symbols(data_out.data(), encoder.block_size());

    // Check if we properly decoded the data
    if (std::equal(data_out.begin(), data_out.end(), data_in.begin()))
    {
        std::cout << "Data decoded correctly" << std::endl;
    }
    else
    {
        std::cout << "Unexpected failure to decode, "
                  << "please file a bug report :)" << std::endl;
    }

    return 0;
}
