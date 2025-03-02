#pragma once

#include "barretenberg/honk/transcript/transcript.hpp"
#include "barretenberg/plonk/proof_system/proving_key/proving_key.hpp"
#include <cstddef>
#include <memory>

namespace proof_system::honk {

// Currently only one type of work queue operation but there will likely be others related to Sumcheck
enum WorkType { SCALAR_MULTIPLICATION };

// TODO(luke): This Params template parameter is the same type expected by e.g. components of the PCS. Eventually it
// should be replaced by some sort of Flavor concept that contains info about the Field etc. This should be resolved
// at the same time as the similar patterns in Gemini etc.
template <typename Params> class work_queue {

    using CommitmentKey = typename Params::CK;
    using FF = typename Params::Fr;
    using Commitment = typename Params::C;

    struct work_item_info {
        uint32_t num_scalar_multiplications;
    };

    struct work_item {
        WorkType work_type = SCALAR_MULTIPLICATION;
        std::span<FF> mul_scalars;
        std::string label;
    };

  private:
    std::shared_ptr<proof_system::plonk::proving_key> proving_key;
    // TODO(luke): Consider handling all transcript interactions in the prover rather than embedding them in the queue.
    proof_system::honk::ProverTranscript<FF>& transcript;
    CommitmentKey commitment_key;
    std::vector<work_item> work_item_queue;

  public:
    explicit work_queue(std::shared_ptr<proof_system::plonk::proving_key>& proving_key,
                        proof_system::honk::ProverTranscript<FF>& prover_transcript)
        : proving_key(proving_key)
        , transcript(prover_transcript)
        , commitment_key(proving_key->circuit_size,
                         "../srs_db/ignition"){}; // TODO(luke): make this properly parameterized

    work_queue(const work_queue& other) = default;
    work_queue(work_queue&& other) noexcept = default;
    work_queue& operator=(const work_queue& other) = delete;
    work_queue& operator=(work_queue&& other) = delete;
    ~work_queue() = default;

    [[nodiscard]] work_item_info get_queued_work_item_info() const
    {
        uint32_t scalar_mul_count = 0;
        for (const auto& item : work_item_queue) {
            if (item.work_type == WorkType::SCALAR_MULTIPLICATION) {
                ++scalar_mul_count;
            }
        }
        return work_item_info{ scalar_mul_count };
    };

    [[nodiscard]] FF* get_scalar_multiplication_data(size_t work_item_number) const
    {
        size_t count = 0;
        for (const auto& item : work_item_queue) {
            if (item.work_type == WorkType::SCALAR_MULTIPLICATION) {
                if (count == work_item_number) {
                    return const_cast<FF*>(item.mul_scalars.data());
                }
                ++count;
            }
        }
        return nullptr;
    };

    [[nodiscard]] size_t get_scalar_multiplication_size(size_t work_item_number) const
    {
        size_t count = 0;
        for (const auto& item : work_item_queue) {
            if (item.work_type == WorkType::SCALAR_MULTIPLICATION) {
                if (count == work_item_number) {
                    return item.mul_scalars.size();
                }
                ++count;
            }
        }
        return 0;
    };

    void put_scalar_multiplication_data(const Commitment& result, size_t work_item_number)
    {
        size_t count = 0;
        for (const auto& item : work_item_queue) {
            if (item.work_type == WorkType::SCALAR_MULTIPLICATION) {
                if (count == work_item_number) {
                    transcript.send_to_verifier(item.label, result);
                    return;
                }
                ++count;
            }
        }
    };

    void flush_queue() { work_item_queue = std::vector<work_item>(); };

    void add_commitment(std::span<FF> polynomial, std::string label)
    {
        add_to_queue({ SCALAR_MULTIPLICATION, polynomial, label });
    }

    void process_queue()
    {
        for (const auto& item : work_item_queue) {
            switch (item.work_type) {

            case WorkType::SCALAR_MULTIPLICATION: {

                // Run pippenger multi-scalar multiplication.
                auto commitment = commitment_key.commit(item.mul_scalars);

                transcript.send_to_verifier(item.label, commitment);

                break;
            }
            default: {
            }
            }
        }
        work_item_queue = std::vector<work_item>();
    };

    [[nodiscard]] std::vector<work_item> get_queue() const { return work_item_queue; };

  private:
    void add_to_queue(const work_item& item)
    {
        // Note: currently no difference between wasm and native but may be in the future
#if defined(__wasm__)
        work_item_queue.push_back(item);
#else
        work_item_queue.push_back(item);
#endif
    };
};
} // namespace proof_system::honk