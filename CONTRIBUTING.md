### Types of Contributions:
Contributions are welcome at various levels:
- Rectifying typos in code, comments, documentation etc..
- Bug fixes
- Extensions such as the addition of new file types for reading/writing
- Adaptations involving new parameters in processing and/or dialogs
- New features - however, these will be evaluated to ensure alignment with the Cartool ecosystem and pipeline

### Programming Guidelines:
- Employ linting tools, whether from Visual Studio or other sources, and diligently adhere to their suggestions.
- Before proposing a merge into the main branch, ensure rigorous testing of all contributions.
- Conduct integration tests, too, as part of the submission process.
- Do not submit code authored by others without proper rights.
- Strictly no AI-generated code; authors must write, test, and retain rights to their contributions.

### Coding Style:
- Attempt to adhere to the current style until a suitable automatic formatting setup is established.
- Align code following the [Ratliff braces style](https://en.wikipedia.org/wiki/Indentation_style#Ratliff_style).
- Provide meaningful comments, particularly in sensitive sections.
- Concentrate both code and comments solely on science and software-related aspects.
- Use spaces, not tab characters, for identation.
- Separate methods with a full horizontal comment line.
- Local variables should be shorter, in lower case, and can optionally make use of snake_case.
- Fields and methods names can be longer, use the [Pascal-case naming convention](https://en.wikipedia.org/wiki/Naming_convention_(programming)) (e.g., ThisIsAField), avoiding snake_case.
- As a general guideline, it is recommended to use the English language for both code and comments.
- Adhere to file naming conventions:
    - Files with a single method / class should be named directly after it, like "MyClass.h" and/or "MyClass.cpp".
    - Some files currently also include a prefix if they belong to a module, like "Electrodes.MyClass.h" "Electrodes.MyProc.cpp" 
    - Methods could be split into multiple files, like "MyClass.Method1.cpp", "MyClass.Method2.cpp" etc...
