// AngelScript translation of BP_ThirdPersonCharacter Blueprint
// Generated from Tests\code.json N2C format

class ABP_ThirdPersonCharacter : ACharacter
{
    // Input Actions (references to Input Action assets)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    UInputAction IA_Jump;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    UInputAction IA_Move;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    UInputAction IA_Look;
    
    // Input Mapping Context
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
    UInputMappingContext IMC_Default;

    // Constructor - Initialize default values
    ABP_ThirdPersonCharacter()
    {
        // Set default mapping context reference
        // IMC_Default would be set to the asset path in Blueprint
    }

    // Event: Called when controller changes (N17 - Event Receive Controller Changed)
    UFUNCTION(BlueprintOverride)
    void ReceiveControllerChanged(AController OldController, AController NewController)
    {
        // Cast new controller to PlayerController (N14)
        APlayerController PlayerController = Cast<APlayerController>(NewController);
        if (PlayerController != nullptr)
        {
            // Get Enhanced Input Local Player Subsystem (N16)
            UEnhancedInputLocalPlayerSubsystem EnhancedInputSubsystem = 
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController.GetLocalPlayer());
            
            if (EnhancedInputSubsystem != nullptr)
            {
                // Add mapping context with priority 0 (N15)
                FModifyContextOptions Options;
                Options.bIgnoreAllPressedKeysUntilRelease = true;
                Options.bForceImmediately = false;
                Options.bNotifyUserSettings = false;
                
                EnhancedInputSubsystem.AddMappingContext(IMC_Default, 0, Options);
            }
        }
    }

    // Setup input component binding (conceptual setup)
    UFUNCTION(BlueprintOverride)
    void SetupPlayerInputComponent(UInputComponent PlayerInputComponent)
    {
        Super::SetupPlayerInputComponent(PlayerInputComponent);
        
        UEnhancedInputComponent EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
        if (EnhancedInput != nullptr)
        {
            // Bind jump actions (N11 - IA_Jump)
            EnhancedInput.BindAction(IA_Jump, ETriggerEvent::Started, this, n"OnJumpStarted");
            EnhancedInput.BindAction(IA_Jump, ETriggerEvent::Completed, this, n"OnJumpCompleted");
            
            // Bind movement action (N12 - IA_Move)
            EnhancedInput.BindAction(IA_Move, ETriggerEvent::Triggered, this, n"OnMoveTriggered");
            
            // Bind look action (N13 - IA_Look)
            EnhancedInput.BindAction(IA_Look, ETriggerEvent::Triggered, this, n"OnLookTriggered");
        }
    }

    // Input Event: Jump action started (N11 -> N1)
    UFUNCTION()
    void OnJumpStarted(const FInputActionValue& Value)
    {
        // Start jumping (N1 - Jump)
        Jump();
    }
    
    // Input Event: Jump action completed (N11 -> N7)
    UFUNCTION()
    void OnJumpCompleted(const FInputActionValue& Value)
    {
        // Stop jumping (N7 - Stop Jumping)
        StopJumping();
    }

    // Input Event: Move action triggered (N12 -> N5 -> N4)
    UFUNCTION()
    void OnMoveTriggered(const FInputActionValue& Value)
    {
        // Get movement input values from action
        FVector2D MovementVector = Value.Get<FVector2D>();
        
        if (Controller != nullptr)
        {
            // Get control rotation for movement direction (N6, N10 - Get Control Rotation)
            FRotator ControlRotation = GetControlRotation();
            
            // Calculate forward movement (N8 - Get Forward Vector)
            FVector ForwardDirection = UKismetMathLibrary::GetForwardVector(ControlRotation);
            AddMovementInput(ForwardDirection, MovementVector.Y); // N4 - Add Movement Input
            
            // Calculate right movement (N9 - Get Right Vector) 
            FVector RightDirection = UKismetMathLibrary::GetRightVector(ControlRotation);
            AddMovementInput(RightDirection, MovementVector.X); // N5 - Add Movement Input
        }
    }

    // Input Event: Look action triggered (N13 -> N3 -> N2)
    UFUNCTION()
    void OnLookTriggered(const FInputActionValue& Value)
    {
        // Get look input values from action
        FVector2D LookAxisVector = Value.Get<FVector2D>();
        
        if (Controller != nullptr)
        {
            // Add yaw input for horizontal mouse movement (N3 - Add Controller Yaw Input)
            AddControllerYawInput(LookAxisVector.X);
            
            // Add pitch input for vertical mouse movement (N2 - Add Controller Pitch Input)
            AddControllerPitchInput(LookAxisVector.Y);
        }
    }
}

/*
EXECUTION FLOW MAPPING FROM JSON:
- N17 (Event Receive Controller Changed) -> N14 (Cast To PlayerController) -> N15 (Add Mapping Context)
- N11 (IA_Jump Started) -> N1 (Jump)
- N11 (IA_Jump Completed) -> N7 (Stop Jumping)  
- N12 (IA_Move Triggered) -> N5 (Add Movement Input - Right) -> N4 (Add Movement Input - Forward)
- N13 (IA_Look Triggered) -> N3 (Add Controller Yaw Input) -> N2 (Add Controller Pitch Input)

DATA FLOW MAPPING FROM JSON:
- N6.P4 (Control Rotation Z/Yaw) -> N8.P3 (Get Forward Vector input)
- N8.P4 (Forward Vector output) -> N4.P4 (Movement direction input)
- N9.P4 (Right Vector output) -> N5.P4 (Movement direction input)
- N10.P2 (Control Rotation X/Roll) -> N9.P1 (Get Right Vector roll input)
- N10.P4 (Control Rotation Z/Yaw) -> N9.P3 (Get Right Vector yaw input)
- N12.P6 (IA_Move Action Value X) -> N5.P5 (Movement scale value)
- N12.P7 (IA_Move Action Value Y) -> N4.P5 (Movement scale value)
- N13.P6 (IA_Look Action Value X) -> N3.P4 (Yaw input value)
- N13.P7 (IA_Look Action Value Y) -> N2.P4 (Pitch input value)
- N14.P5 (PlayerController output) -> N16.P1 (Subsystem target input)
- N16.P2 (Enhanced Input Subsystem output) -> N15.P3 (Add mapping context target)
- N17.P4 (New Controller output) -> N14.P4 (Cast object input)

ANGELSCRIPT SPECIFIC FEATURES USED:
- Class inheritance with ACharacter base class
- UPROPERTY macros for Blueprint integration
- UFUNCTION macros with BlueprintOverride
- Cast<> template for type casting
- Enhanced Input Component binding patterns
- FName literals with n"" syntax
- Unreal Engine specific types (FVector2D, FRotator, etc.)
*/