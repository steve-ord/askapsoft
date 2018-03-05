/* This file was generated by ODB, object-relational mapping (ORM)
 * compiler for C++.
 */

DROP TABLE IF EXISTS "Polarisation";

CREATE TABLE IF NOT EXISTS "schema_version" (
  "name" TEXT NOT NULL PRIMARY KEY,
  "version" INTEGER NOT NULL,
  "migration" INTEGER NOT NULL);

DELETE FROM "schema_version"
  WHERE "name" = '';

CREATE TABLE "Polarisation" (
  "version" INTEGER NOT NULL,
  "polarisation_component_id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
  "component_id" TEXT NOT NULL,
  "flux_I_median" REAL NOT NULL,
  "flux_Q_median" REAL NOT NULL,
  "flux_U_median" REAL NOT NULL,
  "flux_V_median" REAL NOT NULL,
  "rms_I" REAL NOT NULL,
  "rms_Q" REAL NOT NULL,
  "rms_U" REAL NOT NULL,
  "rms_V" REAL NOT NULL,
  "co_1" REAL NOT NULL,
  "co_2" REAL NOT NULL,
  "co_3" REAL NOT NULL,
  "co_4" REAL NOT NULL,
  "co_5" REAL NOT NULL,
  "lambda_ref_sq" REAL NOT NULL,
  "rmsf_fwhm" REAL NOT NULL,
  "pol_peak" REAL NOT NULL,
  "pol_peak_debias" REAL NOT NULL,
  "pol_peak_err" REAL NOT NULL,
  "pol_peak_fit" REAL NOT NULL,
  "pol_peak_fit_debias" REAL NOT NULL,
  "pol_peak_fit_err" REAL NOT NULL,
  "pol_peak_fit_snr" REAL NOT NULL,
  "pol_peak_fit_snr_err" REAL NOT NULL,
  "fd_peak" REAL NOT NULL,
  "fd_peak_err" REAL NOT NULL,
  "fd_peak_fit" REAL NOT NULL,
  "fd_peak_fit_err" REAL NOT NULL,
  "pol_ang_ref" REAL NOT NULL,
  "pol_ang_ref_err" REAL NOT NULL,
  "pol_ang_zero" REAL NOT NULL,
  "pol_ang_zero_err" REAL NOT NULL,
  "pol_frac" REAL NOT NULL,
  "pol_frac_err" REAL NOT NULL,
  "complex_1" REAL NOT NULL,
  "complex_2" REAL NOT NULL,
  "flag_p1" INT NOT NULL,
  "flag_p2" INT NOT NULL,
  "flag_p3" INT NOT NULL,
  "flag_p4" INT NOT NULL);

CREATE INDEX "Polarisation_polarisation_component_id_i"
  ON "Polarisation" ("polarisation_component_id");

INSERT OR IGNORE INTO "schema_version" (
  "name", "version", "migration")
  VALUES ('', 2, 0);
