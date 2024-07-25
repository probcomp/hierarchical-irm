import pandas
# Short script for reading in the flight data and converting them in to an HIRM compatible format.

def main():
  df = pandas.read_csv('flights_clean.csv')

  # We will write out 3 separate datasets.
  # The first will be a CrossCat format dataset where the only Domain is rows,
  # and every column is a unary relation.

  # The second only contains the source and flight name as domains.
  # There is a binary relation on whether a source reports on a flight.

  # Finally the third is a list of flights (1 Domain which is the flight name,
  # and a string relation for the name).

  # NOTE: HIRM schema expects space separated data, hence we hyphenate.

  f = open('pclean_flights_clean_unary.obs', 'w')

  # Write out value, name of field, record # for HIRM.
  for index, row in df.iterrows():
    for col_name in df.columns:
      if col_name == 'tuple_id':
        continue
      new_col_name = col_name.replace(' ', '-')
      f.write(f"{row[col_name]},{new_col_name},{row['tuple_id']}\n")
  f.close()

  f = open('pclean_flights_clean_source_flight_name.obs', 'w')

  for index, row in df.iterrows():
    f.write(f"1,reports_on,{row['src']},{row['flight']}\n")
  f.close()

  f = open('pclean_flights_clean_flight_name.obs', 'w')
  for name in df['flight'].unique():
    f.write(f"{name},has_name,{name}\n")
  f.close()

main()
