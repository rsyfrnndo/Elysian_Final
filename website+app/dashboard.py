import pandas as pd
import streamlit as st
from sklearn.model_selection import train_test_split
from sklearn.linear_model import LinearRegression
from sklearn.metrics import mean_absolute_error, mean_squared_error, r2_score

before_df = pd.read_csv('../dataset/mwt.before.csv')
after_df = pd.read_csv('../dataset/mwt.after.csv')

merged_df = before_df.merge(after_df, on='id', suffixes=('_before', '_after'))
merged_df['persentase'] = ((merged_df['ppm_before'] - merged_df['ppm_after']) / merged_df['ppm_before']) * 100

X = merged_df[['ppm_before', 'ppm_after']]
y = merged_df['persentase']

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)
linear_model = LinearRegression()
linear_model.fit(X_train, y_train)
y_pred = linear_model.predict(X_test)

r2 = r2_score(y_test, y_pred)

st.title("Filtration Effectiveness Dashboard")

# Create three columns
last_row = merged_df.iloc[-1]
persentase = "{:.0%}".format(r2)
ril_persen = f"{r2}%"
col1, col2, col3 = st.columns(3)

with col1:
    st.subheader("PPM Before Filtration")
    st.metric(label="PPM Sebelum", value=f"{last_row['ppm_before']}")

with col2:
    st.subheader("PPM After Filtration")
    st.metric(label="PPM Sesudah", value=f"{last_row['ppm_after']}")

with col3:
    st.subheader("Effectiveness Percentage")
    st.metric(label="Efektivitas", value=persentase)
    st.write(ril_persen)
