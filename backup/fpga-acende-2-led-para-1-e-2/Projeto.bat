@echo off

cd /d Projeto/impl1

openFPGALoader -b colorlight-i9 --unprotect-flash -f --verify Projeto_impl1.bit