# wakeUP_Computer
This project was undertaken as a criterion for evaluation in the subject of Operating Systems 2 at UFRGS. The objective of the work is to implement a "sleep management" service for workstations that belong to the same physical network segment of a large organization. 

# Tutorial de como Rodar o programa

Para a compilação e execução elaboramos um script .sh para automatizar as tarefas de compilação e execução. Aqui está o tutorial de como rodar o programa via script localizado no arquivo ZIP em anexo e como nome `run_program.sh`:

1. Abra o terminal no Ubuntu (pressionando "Ctrl + Alt + T" no teclado).
2. Navegue até a pasta onde o script "run_program.sh" está salvo.
3. Verifique se o script "run_program.sh" tem permissão para ser executado. Se você ainda não concedeu permissão, use o seguinte comando para dar permissão de execução:
```
chmod +x run_program.sh
```
4. Agora você pode executar o script com ou sem o parâmetro "manager". 
- Para executar o script como participante, basta digitar o seguinte comando:
  ```
  ./run_program.sh
  ```
- Se você quiser executar o script como "manager", basta digitar o seguinte comando:
  ```
  ./run_program.sh manager
  ```
