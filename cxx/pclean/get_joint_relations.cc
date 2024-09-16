#include "pclean/get_joint_relations.hh"

#include <map>
#include <string>
#include <utility>

#include "distributions/get_distribution.hh"
#include "emissions/get_emission.hh"

std::map<std::string, std::pair<std::string, std::string>> JOINT_NAME_TO_PARTS =
{
  // TODO(thomaswc): Implement an int emission function to use with the
  // Skellam distribution.
  {"bool", {"bernoulli", "sometimes_bitflip"}},
  {"categorical", {"categorical", "sometimes_categorical"}},
  {"real", {"normal", "sometimes_gaussian"}},
  {"string", {"bigram", "bigram"}},
  {"stringcat", {"stringcat", "bigram"}},
  {"typo_int", {"string_skellam", "bigram"}},
  {"typo_nat", {"string_nat", "bigram"}},
  {"typo_real", {"string_normal", "bigram"}},
  // TODO(thomaswc): Consider implementing an emission function to use with
  // stringcat where the corrupted value is still a valid, in-category string.
  // TODO(thomaswc): Consider implementing a pair for positive reals based
  // on a log-normal distribution.
};

T_clean_relation get_distribution_relation(
    const ScalarVar& scalar_var,
    const std::vector<std::string>& domains) {
  T_clean_relation cr;
  if (!JOINT_NAME_TO_PARTS.contains(scalar_var.joint_name)) {
    printf("Unknown joint distribution name %s\n", scalar_var.joint_name.c_str());
    std::exit(1);
  }
  cr.distribution_spec = DistributionSpec(
      JOINT_NAME_TO_PARTS.at(scalar_var.joint_name).first,
      scalar_var.params);
  cr.is_observed = false;
  cr.domains = domains;
  return cr;
}

T_noisy_relation get_emission_relation(
    const ScalarVar& scalar_var,
    const std::vector<std::string>& domains,
    const std::string& base_relation) {
  T_noisy_relation nr;
  if (!JOINT_NAME_TO_PARTS.contains(scalar_var.joint_name)) {
    printf("Unknown joint distribution name %s\n", scalar_var.joint_name.c_str());
    std::exit(1);
  }
  nr.emission_spec = EmissionSpec(
      JOINT_NAME_TO_PARTS.at(scalar_var.joint_name).second,
      scalar_var.params);
  nr.is_observed = false;
  nr.domains = domains;
  nr.base_relation = base_relation;
  return nr;
}
