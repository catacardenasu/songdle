import ast
import pandas as pd

df = pd.read_csv("data/raw/tracks.csv")

def section(title):
    print("\n" + "=" * 50)
    print(title)
    print("=" * 50)

section("SIZE AND COLUMNS")
print("Rows:", len(df))
print("Columns:", df.columns.tolist())

section("SAMPLE ROWS")
print(df[["name", "artists", "release_date", "popularity", "tempo"]].head(10).to_string())

section("MISSING VALUES")
print(df[["name", "artists", "release_date", "popularity", "tempo"]].isna().sum())

section("RELEASE DATE FORMATS (length: count)")
print("4 = year only, 7 = year-month, 10 = full date")
print(df["release_date"].astype(str).str.len().value_counts())

section("YEAR RANGE")
years = df["release_date"].astype(str).str[:4].astype(int)
print("Earliest:", years.min(), " Latest:", years.max())

section("ARTISTS")
def artist_count(s):
    try:
        return len(ast.literal_eval(s))
    except Exception:
        return -1
counts = df["artists"].astype(str).apply(artist_count)
print("Songs with 1 artist:", (counts == 1).sum())
print("Songs with 2+ artists:", (counts >= 2).sum())
print("Couldn't parse:", (counts == -1).sum())

section("TEMPO")
print(df["tempo"].describe())
print("Tempo == 0:", (df["tempo"] == 0).sum())

section("POPULARITY")
print(df["popularity"].describe())
print("Popularity == 0:", (df["popularity"] == 0).sum())

section("DUPLICATES")
print("Same name + artists:", df.duplicated(subset=["name", "artists"]).sum())

section("MYSTERY SONG POOL (songs at or above cutoff)")
for cutoff in [30, 40, 50, 60, 70]:
    print(f"popularity >= {cutoff}: {(df['popularity'] >= cutoff).sum()}")