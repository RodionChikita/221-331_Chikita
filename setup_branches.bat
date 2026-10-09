@echo off

echo Перед запуском убедитесь, что скопировали новые файлы (Lab2\, Lab3\,
echo обновлённые README.md и .gitignore) в свой локальный клон репозитория.
pause

git checkout main
git checkout -b Lab2
git add Lab2 README.md .gitignore
git commit -m "Lab2: transparent AES-256 disk I/O encryption minifilter driver"
git push -u origin Lab2

git checkout main
git merge Lab2
git push origin main

git checkout -b Lab3
git add Lab3 README.md
git commit -m "Lab3: Intel SGX enclave secure storage (simulation mode)"
git push -u origin Lab3

git checkout main
git merge Lab3
git push origin main

echo Готово. Проверьте ветки: git branch -a
