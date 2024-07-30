#define BOOST_TEST_MODULE test pclean_get_joint_relations

#include "pclean/get_joint_relations.hh"
#include <boost/test/included/unit_test.hpp>
namespace tt = boost::test_tools;

BOOST_AUTO_TEST_CASE(test_bool) {
  ScalarVar sv;
  sv.joint_name = "bool";
  T_clean_relation cr = get_distribution_relation(sv, {"City", "College"});
  BOOST_TEST(!cr.is_observed);
  std::vector<std::string> expected_domains = {"City", "College"};
  BOOST_TEST(cr.domains == expected_domains);
  BOOST_TEST((cr.distribution_spec.distribution == DistributionEnum::bernoulli));

  T_noisy_relation nr = get_emission_relation(sv, {"City", "College"}, "R1");
  BOOST_TEST(!nr.is_observed);
  BOOST_TEST(nr.domains == expected_domains);
  BOOST_TEST((nr.emission_spec.emission == EmissionEnum::sometimes_bitflip));
  BOOST_TEST(nr.base_relation == "R1");
}

BOOST_AUTO_TEST_CASE(test_categorical) {
  ScalarVar sv;
  sv.joint_name = "categorical";
  sv.params["k"] = "5";
  T_clean_relation cr = get_distribution_relation(sv, {"Employer", "Country"});
  BOOST_TEST(!cr.is_observed);
  std::vector<std::string> expected_domains = {"Employer", "Country"};
  BOOST_TEST(cr.domains == expected_domains);
  BOOST_TEST((cr.distribution_spec.distribution == DistributionEnum::categorical));

  T_noisy_relation nr = get_emission_relation(sv, {"Employer", "Country"}, "R2");
  BOOST_TEST(!nr.is_observed);
  BOOST_TEST(nr.domains == expected_domains);
  BOOST_TEST((nr.emission_spec.emission == EmissionEnum::sometimes_categorical));
  BOOST_TEST(nr.base_relation == "R2");
}

BOOST_AUTO_TEST_CASE(test_real) {
  ScalarVar sv;
  sv.joint_name = "real";
  T_clean_relation cr = get_distribution_relation(sv, {"Parent"});
  BOOST_TEST(!cr.is_observed);
  std::vector<std::string> expected_domains = {"Parent"};
  BOOST_TEST(cr.domains == expected_domains);
  BOOST_TEST((cr.distribution_spec.distribution == DistributionEnum::normal));

  T_noisy_relation nr = get_emission_relation(sv, {"Parent"}, "R3");
  BOOST_TEST(!nr.is_observed);
  BOOST_TEST(nr.domains == expected_domains);
  BOOST_TEST((nr.emission_spec.emission == EmissionEnum::sometimes_gaussian));
  BOOST_TEST(nr.base_relation == "R3");
}

BOOST_AUTO_TEST_CASE(test_string) {
  ScalarVar sv;
  sv.joint_name = "string";
  T_clean_relation cr = get_distribution_relation(sv, {"Publisher", "City"});
  BOOST_TEST(!cr.is_observed);
  std::vector<std::string> expected_domains = {"Publisher", "City"};
  BOOST_TEST(cr.domains == expected_domains);
  BOOST_TEST((cr.distribution_spec.distribution == DistributionEnum::bigram));

  T_noisy_relation nr = get_emission_relation(sv, {"Publisher", "City"}, "R4");
  BOOST_TEST(!nr.is_observed);
  BOOST_TEST(nr.domains == expected_domains);
  BOOST_TEST((nr.emission_spec.emission == EmissionEnum::bigram_string));
  BOOST_TEST(nr.base_relation == "R4");
}

BOOST_AUTO_TEST_CASE(test_stringcat) {
  ScalarVar sv;
  sv.joint_name = "stringcat";
  sv.params["strings"] = "radial bilateral asymmetric";
  T_clean_relation cr = get_distribution_relation(
      sv, {"Species", "Genus", "Family", "Order"});
  BOOST_TEST(!cr.is_observed);
  std::vector<std::string> expected_domains = {
    "Species", "Genus", "Family", "Order"};
  BOOST_TEST(cr.domains == expected_domains);
  BOOST_TEST((cr.distribution_spec.distribution == DistributionEnum::stringcat));

  T_noisy_relation nr = get_emission_relation(
      sv, {"Species", "Genus", "Family", "Order"}, "R5");
  BOOST_TEST(!nr.is_observed);
  BOOST_TEST(nr.domains == expected_domains);
  BOOST_TEST((nr.emission_spec.emission == EmissionEnum::bigram_string));
  BOOST_TEST(nr.base_relation == "R5");
}
