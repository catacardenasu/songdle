import ast
import pandas as pd

df = pd.read_csv("data/raw/tracks.csv")
start = len(df)

# Drop songs with no name
df = df.dropna(subset=["name"])

# Drop songs with no tempo measurement
df = df[df["tempo"] > 0]

# Year: first 4 characters of release_date
df["year"] = df["release_date"].astype(str).str[:4].astype(int)

# Artist: first artist in the list
df["artist"] = df["artists"].apply(lambda s: ast.literal_eval(s)[0])

# Drop duplicates (same name and same artist list)
df = df.drop_duplicates(subset=["name", "artists"])

# Remove tabs and newlines so the TSV stays valid
for col in ["name", "artist"]:
    df[col] = df[col].astype(str).str.replace(r"[\t\r\n]", " ", regex=True).str.strip()

# Keep only the columns the C++ program needs
out = df[["name", "artist", "year", "popularity", "tempo"]]
out.to_csv("data/songs_clean.tsv", sep="\t", index=False, encoding="utf-8")

print("Rows before:", start)
print("Rows after: ", len(out))
print("Mystery pool (popularity >= 60):", (out["popularity"] >= 60).sum())
print(out.head())