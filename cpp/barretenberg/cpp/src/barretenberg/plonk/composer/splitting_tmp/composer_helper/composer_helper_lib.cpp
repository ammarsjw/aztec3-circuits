/**
 * @file composer_helper_lib.cpp
 * @brief Contains implementations of some of the functions used both by Honk and Plonk-style composers (excluding
 * permutation functions)
 *
 */
#include "composer_helper_lib.hpp"
#include "barretenberg/honk/pcs/commitment_key.hpp"

namespace proof_system::plonk {

/**
 * @brief Retrieve lagrange forms of selector polynomials and compute monomial and coset-monomial forms and put into
 * cache
 *
 * @param key Pointer to the proving key
 * @param selector_properties Names of selectors
 */
void compute_monomial_and_coset_selector_forms(plonk::proving_key* circuit_proving_key,
                                               std::vector<SelectorProperties> selector_properties)
{
    for (size_t i = 0; i < selector_properties.size(); i++) {
        // Compute monomial form of selector polynomial
        auto& selector_poly_lagrange =
            circuit_proving_key->polynomial_store.get(selector_properties[i].name + "_lagrange");
        barretenberg::polynomial selector_poly(circuit_proving_key->circuit_size);
        barretenberg::polynomial_arithmetic::ifft(
            &selector_poly_lagrange[0], &selector_poly[0], circuit_proving_key->small_domain);

        // Compute coset FFT of selector polynomial
        barretenberg::polynomial selector_poly_fft(selector_poly, circuit_proving_key->circuit_size * 4 + 4);
        selector_poly_fft.coset_fft(circuit_proving_key->large_domain);

        // TODO(luke): For Standard/Turbo, the lagrange polynomials can be removed from the store at this point but this
        // is not the case for Ultra. Implement?
        circuit_proving_key->polynomial_store.put(selector_properties[i].name, std::move(selector_poly));
        circuit_proving_key->polynomial_store.put(selector_properties[i].name + "_fft", std::move(selector_poly_fft));
    }
}

/**
 * @brief Computes the verification key by computing the:
 * (1) commitments to the selector, permutation, and lagrange (first/last) polynomials,
 * (2) sets the polynomial manifest using the data from proving key.
 */
std::shared_ptr<plonk::verification_key> compute_verification_key_common(
    std::shared_ptr<plonk::proving_key> const& proving_key, std::shared_ptr<VerifierReferenceString> const& vrs)
{
    auto circuit_verification_key = std::make_shared<plonk::verification_key>(
        proving_key->circuit_size, proving_key->num_public_inputs, vrs, proving_key->composer_type);
    // TODO(kesha): Dirty hack for now. Need to actually make commitment-agnositc
    auto commitment_key = proof_system::honk::pcs::kzg::CommitmentKey(proving_key->circuit_size, "../srs_db/ignition");

    for (size_t i = 0; i < proving_key->polynomial_manifest.size(); ++i) {
        const auto& poly_info = proving_key->polynomial_manifest[i];

        const std::string poly_label(poly_info.polynomial_label);
        const std::string selector_commitment_label(poly_info.commitment_label);

        if (poly_info.source == PolynomialSource::SELECTOR || poly_info.source == PolynomialSource::PERMUTATION ||
            poly_info.source == PolynomialSource::OTHER) {
            // Fetch the polynomial in its vector form.

            // Commit to the constraint selector polynomial and insert the commitment in the verification key.

            auto poly_commitment = commitment_key.commit(proving_key->polynomial_store.get(poly_label));
            circuit_verification_key->commitments.insert({ selector_commitment_label, poly_commitment });
        }
    }

    // Set the polynomial manifest in verification key.
    circuit_verification_key->polynomial_manifest = proof_system::plonk::PolynomialManifest(proving_key->composer_type);

    return circuit_verification_key;
}

} // namespace proof_system::plonk
