import pandas
# Short script for reading in the rental data and converting them in to an HIRM compatible format.

def main():
  df = pandas.read_csv('rents_clean.csv')

  # We will write out 3 separate datasets.
  # The first will be a CrossCat format dataset where the only Domain is rows,
  # and every column is a unary relation.

  # The second is a more informed dataset where we have 3 Domains:
  # row, County and State. Monthly Rent and Room Type are ternary relations
  # on these three domains.

  # Finally the third is a list of counties (1 Domain which is the county, and
  # a string relation for the name).

  # Write out value, name of field, record # for HIRM.

  # NOTE: HIRM expects space separated schema, hence we hyphenate.
  f = open('pclean_rents_clean_unary.obs', 'w')
  for index, row in df.iterrows():
    for col_name in df.columns:
      if col_name == 'ID':
        continue
      value = row[col_name]
      new_col_name = col_name.replace(' ', '-')
      f.write(f"{row[col_name]},{new_col_name},{row['ID']}\n")
  f.close()

  f = open('pclean_rents_clean_ternary.obs', 'w')
  for index, row in df.iterrows():
    county = row['County']
    f.write(f"{row['Monthly Rent']},has_rent,{row['ID']},{county},{row['State']}\n")
    f.write(f"{row['Room Type']},has_type,{row['ID']},{county} {row['State']}\n")
    f.write(f"{county},has_name,{county},{row['State']}\n")
  f.close()

  f = open('pclean_rents_clean_county_names.obs', 'w')
  for name in df['County'].unique():
    name = name.replace(' ', '-')
    f.write(f"{name},has_name,{name}\n")
  f.close()

main()
