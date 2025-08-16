# Unreal Engine 5.5 Development Notes

This document captures important lessons learned while developing the NodeToCode plugin for UE 5.5, particularly around API changes and compilation requirements.

## Asset Registry API Changes

### GetAssetsByClass Parameter Changes
**Issue**: `IAssetRegistry::GetAssetsByClass()` no longer accepts `FName` for the class parameter.

**Old UE 5.4 syntax**:
```cpp
AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), BlueprintAssets);
```

**New UE 5.5 syntax**:
```cpp
AssetRegistry.GetAssetsByClass(UBlueprint::StaticClass()->GetClassPathName(), BlueprintAssets);
```

### Asset Data Class Path Changes
**Issue**: `FAssetData.AssetClass` has been replaced with `FAssetData.AssetClassPath`.

**Old UE 5.4 syntax**:
```cpp
if (AssetData.AssetClass != UBlueprint::StaticClass()->GetFName())
```

**New UE 5.5 syntax**:
```cpp
if (AssetData.AssetClassPath != UBlueprint::StaticClass()->GetClassPathName())
```

## Delegate Binding Changes

### Template Deduction Stricter Requirements
**Issue**: UE 5.5 has much stricter template requirements for delegate binding, especially with lambda functions.

**Problem**: Lambda captures in delegate binding cause template deduction failures:
```cpp
// This fails in UE 5.5:
EditorStartupHandle = FEditorDelegates::OnEditorInitialized.AddLambda([this]()
{
    OnEditorStartup();
});
```

**Solution**: Use static wrapper functions for delegate binding:
```cpp
// Header file:
static void StaticOnEditorStartup(double);

// Implementation:
void FN2CTranslatorManager::StaticOnEditorStartup(double)
{
    FN2CTranslatorManager::Get().OnEditorStartup();
}

// Binding:
EditorStartupHandle = FEditorDelegates::OnEditorInitialized.AddStatic(&FN2CTranslatorManager::StaticOnEditorStartup);
```

### Delegate Signature Discovery
**Important**: Always check the actual delegate signature before binding. Some delegates have unexpected parameters.

**Example**: `FEditorDelegates::OnEditorInitialized` takes a `double` parameter (likely timing-related), not void as one might expect.

**Method**: Use compilation error messages to determine correct signature:
```
error C2664: '...AddStatic<>(void (__cdecl *)(double))': cannot convert argument 1 from 'void (__cdecl *)(void)' to 'void (__cdecl *)(double)'
```

This error clearly shows the delegate expects `void function(double)`.

## LOCTEXT Namespace Management

### Proper Namespace Scope
**Issue**: `LOCTEXT_NAMESPACE` must be properly scoped to avoid "undeclared identifier" errors.

**Problem**: Undefining `LOCTEXT_NAMESPACE` too early:
```cpp
#undef LOCTEXT_NAMESPACE // Too early!

// Later code that uses LOCTEXT will fail:
LOCTEXT("Key", "Value") // Error: LOCTEXT_NAMESPACE undeclared
```

**Solution**: Keep `LOCTEXT_NAMESPACE` defined until all LOCTEXT usage is complete:
```cpp
// All LOCTEXT usage here
LOCTEXT("TranslatorSettingsName", "Node to Code Translator");

// Only undefine at the very end of the file
#undef LOCTEXT_NAMESPACE
```

## Build System Considerations
Type this command line to build the editor with verbose output:
```cmd
"C:\UE\UE_5.5_AS_2\Engine\Build\BatchFiles\RunUAT.bat" \ -ScriptsForProject="C:\Projects\Unreal\Blank_55\Blank_55.uproject" BuildEditor -Verbose \ -project="C:\Projects\Unreal\Blank_55\Blank_55.uproject" -notools
```

### Build Output Interpretation
**Success indicators**:
```
Target is up to date
Total execution time: X.XX seconds
BUILD SUCCESSFUL
ExitCode=0 (Success)
```

**Failure indicators**:
```
BUILD FAILED
ExitCode=8 (or other non-zero)
Compilation errors with specific file/line numbers
```

### Error Message Interpretation
**Learning**: UE 5.5 template error messages can be extremely verbose and nested. Focus on:
1. The innermost error message (usually the actual problem)
2. The line number where the error originates
3. Template parameter mismatches in the error text

## Architecture Patterns for UE 5.5

### Singleton Pattern with Delegates
**Pattern**: When using singleton classes with UE delegates, prefer static wrapper functions over member function binding to avoid template complications.

**Example**:
```cpp
class FMyManager
{
public:
    static FMyManager& Get();
    
    // Public interface
    void Initialize();
    
private:
    // Static wrapper for delegates
    static void StaticOnSomeEvent(FRequiredParams params);
    
    // Actual implementation
    void OnSomeEvent(FRequiredParams params);
};
```

### Non-UObject Delegate Binding
**Key Learning**: For classes that don't inherit from `UObject`, delegate binding options are more limited:
- ❌ `AddUObject` - Only for UObject-derived classes
- ✅ `AddStatic` - For static functions (preferred for singletons)
- ⚠️ `AddRaw` - Can work but has stricter template requirements in UE 5.5
- ⚠️ `AddLambda` - Prone to template deduction issues in UE 5.5

## Debugging Tips

### Template Error Debugging
1. Start with the simplest possible function signature
2. Use static functions to eliminate `this` pointer complications
3. Check delegate documentation for exact signature requirements
4. Use the compilation error messages to understand expected vs actual types

### Asset Registry Debugging
1. Use `FAssetData::IsValid()` to verify asset data
2. Check both `AssetClassPath` and `PackageName` for filtering
3. Verify asset loading with `AssetData.GetAsset()` returns valid pointers

## Version Migration Checklist

When upgrading plugins from UE 5.4 to 5.5:

- [ ] Replace `GetAssetsByClass(FName)` with `GetAssetsByClass(FTopLevelAssetPath)`
- [ ] Replace `AssetData.AssetClass` with `AssetData.AssetClassPath`
- [ ] Review all delegate bindings, especially lambda usage
- [ ] Test compilation with verbose output to catch ignored plugins
- [ ] Verify LOCTEXT_NAMESPACE scope throughout files
- [ ] Update any deprecated API calls based on compilation warnings

## Conclusion

UE 5.5 represents a significant step toward stricter C++ template compliance and improved type safety. While this makes some code more verbose (especially delegate binding), it results in more robust and maintainable plugins. The key is to work with the type system rather than against it, using static wrapper functions and proper API calls as the engine expects.