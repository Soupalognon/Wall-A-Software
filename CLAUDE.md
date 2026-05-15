# Instructions projet — Wall-A-Software

## Lancer les tests

Ne jamais tenter de lancer les tests via les outils Bash ou PowerShell.
Les tests utilisent MSYS2/MinGW64 et la sortie n'est pas capturée correctement.

À la place : demander à l'utilisateur de lancer lui-même la commande suivante depuis PowerShell, puis coller le résultat :

```powershell
& "D:\msys64\usr\bin\bash.exe" -l "D:\_Programs\STM32\Wall-A-Software\Wall-A-STM\Tests\run_tests.sh"
```
