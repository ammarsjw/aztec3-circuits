#pragma once
#include "barretenberg/plonk/proof_system/proving_key/proving_key.hpp"
#include "../types/proof.hpp"
#include "../types/program_settings.hpp"
#include "../widgets/random_widgets/random_widget.hpp"
#include "../../../proof_system/work_queue/work_queue.hpp"
#include "../widgets/transition_widgets/transition_widget.hpp"
#include "../commitment_scheme/commitment_scheme.hpp"

namespace proof_system::plonk {

template <typename settings> class ProverBase {

  public:
    ProverBase(std::shared_ptr<proving_key> input_key = nullptr,
               const transcript::Manifest& manifest = transcript::Manifest());
    ProverBase(ProverBase&& other);
    ProverBase(const ProverBase& other) = delete;
    ProverBase& operator=(const ProverBase& other) = delete;
    ProverBase& operator=(ProverBase&& other);

    void execute_preamble_round();
    void execute_first_round();
    void execute_second_round();
    void execute_third_round();
    void execute_fourth_round();
    void execute_fifth_round();
    void execute_sixth_round();

    void add_polynomial_evaluations_to_transcript();
    void compute_batch_opening_polynomials();
    void compute_wire_commitments();
    void compute_quotient_commitments();
    void init_quotient_polynomials();
    void compute_opening_elements();
    void add_plookup_memory_records_to_w_4();

    void compute_quotient_evaluation();
    void add_blinding_to_quotient_polynomial_parts();
    void compute_lagrange_1_fft();
    plonk::proof& export_proof();
    plonk::proof& construct_proof();

    size_t get_circuit_size() const { return circuit_size; }

    void flush_queued_work_items() { queue.flush_queue(); }

    work_queue::work_item_info get_queued_work_item_info() const { return queue.get_queued_work_item_info(); }

    barretenberg::fr* get_scalar_multiplication_data(const size_t work_item_number) const
    {
        return queue.get_scalar_multiplication_data(work_item_number);
    }

    size_t get_scalar_multiplication_size(const size_t work_item_number) const
    {
        return queue.get_scalar_multiplication_size(work_item_number);
    }

    barretenberg::fr* get_ifft_data(const size_t work_item_number) const
    {
        return queue.get_ifft_data(work_item_number);
    }

    work_queue::queued_fft_inputs get_fft_data(const size_t work_item_number) const
    {
        return queue.get_fft_data(work_item_number);
    }

    void put_scalar_multiplication_data(const barretenberg::g1::affine_element result, const size_t work_item_number)
    {
        queue.put_scalar_multiplication_data(result, work_item_number);
    }

    void put_fft_data(barretenberg::fr* result, const size_t work_item_number)
    {
        queue.put_fft_data(result, work_item_number);
    }

    void put_ifft_data(barretenberg::fr* result, const size_t work_item_number)
    {
        queue.put_ifft_data(result, work_item_number);
    }

    void reset();

    size_t circuit_size;

    std::vector<std::unique_ptr<ProverRandomWidget>> random_widgets;
    std::vector<std::unique_ptr<widget::TransitionWidgetBase<barretenberg::fr>>> transition_widgets;
    transcript::StandardTranscript transcript;

    std::shared_ptr<proving_key> key;
    std::unique_ptr<CommitmentScheme> commitment_scheme;

    work_queue queue;

  private:
    plonk::proof proof;
};
extern template class ProverBase<standard_settings>;
extern template class ProverBase<turbo_settings>;
extern template class ProverBase<ultra_settings>;
extern template class ProverBase<ultra_to_standard_settings>;

typedef ProverBase<standard_settings> Prover;
typedef ProverBase<turbo_settings> TurboProver;
typedef ProverBase<ultra_settings> UltraProver; // TODO(Mike): maybe just return a templated proverbase so that I don't
                                                // need separate cases for ultra vs ultra_to_standard...???
                                                // TODO(Cody): Make this into an issue?
typedef ProverBase<ultra_to_standard_settings> UltraToStandardProver;

} // namespace proof_system::plonk
