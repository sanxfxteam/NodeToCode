// Pseudo-code C# representation of BP_ThirdPersonCharacter Blueprint
// Generated from Tests\code.json N2C format

using UnrealEngine;
using UnrealEngine.InputSystem;

public class BP_ThirdPersonCharacter : Character
{
    // Input Actions (references to Input Action assets)
    private InputAction IA_Jump;
    private InputAction IA_Move; 
    private InputAction IA_Look;
    
    // Input Mapping Context
    private InputMappingContext IMC_Default = "/Game/ThirdPerson/Input/IMC_Default";

    // Event: Called when controller changes (N17)
    protected override void OnControllerChanged(Controller oldController, Controller newController)
    {
        // Cast new controller to PlayerController (N14)
        if (newController is PlayerController playerController)
        {
            // Get Enhanced Input Local Player Subsystem (N16)
            var enhancedInputSubsystem = playerController.GetLocalPlayerSubsystem<EnhancedInputLocalPlayerSubsystem>();
            
            // Add mapping context with priority 0 (N15)
            enhancedInputSubsystem.AddMappingContext(
                mappingContext: IMC_Default,
                priority: 0,
                options: new ModifyContextOptions
                {
                    bIgnoreAllPressedKeysUntilRelease = true,
                    bForceImmediately = false,
                    bNotifyUserSettings = false
                }
            );
        }
    }

    // Input Event: Jump action triggered (N11)
    private void OnJumpStarted()
    {
        // Start jumping (N1)
        this.Jump();
    }
    
    // Input Event: Jump action completed (N11)
    private void OnJumpCompleted()
    {
        // Stop jumping (N7)
        this.StopJumping();
    }

    // Input Event: Move action triggered (N12)
    private void OnMoveTriggered(Vector2 actionValue)
    {
        // Get control rotation for movement direction (N6, N10)
        Rotator controlRotation = this.GetControlRotation();
        
        // Calculate forward movement (N8)
        Vector3 forwardDirection = KismetMathLibrary.GetForwardVector(controlRotation);
        this.AddMovementInput(
            worldDirection: forwardDirection,
            scaleValue: actionValue.Y, // Forward/backward from Y axis
            force: false
        );
        
        // Calculate right movement (N9)
        Vector3 rightDirection = KismetMathLibrary.GetRightVector(controlRotation);
        this.AddMovementInput(
            worldDirection: rightDirection,
            scaleValue: actionValue.X, // Left/right from X axis
            force: false
        );
    }

    // Input Event: Look action triggered (N13)
    private void OnLookTriggered(Vector2 actionValue)
    {
        // Add yaw input for horizontal mouse movement (N3)
        this.AddControllerYawInput(actionValue.X);
        
        // Add pitch input for vertical mouse movement (N2)
        this.AddControllerPitchInput(actionValue.Y);
    }

    // Setup input bindings (conceptual - actual UE5 uses Enhanced Input binding)
    protected virtual void SetupInputBindings()
    {
        // Bind jump actions
        IA_Jump.Started += OnJumpStarted;
        IA_Jump.Completed += OnJumpCompleted;
        
        // Bind movement action
        IA_Move.Triggered += OnMoveTriggered;
        
        // Bind look action  
        IA_Look.Triggered += OnLookTriggered;
    }
}

/*
EXECUTION FLOW ANALYSIS FROM JSON:
- N17 (Controller Changed) -> N14 (Cast to PlayerController) -> N15 (Add Mapping Context)
- N11 (Jump Input) -> N1 (Jump) and N7 (Stop Jump) 
- N12 (Move Input) -> N5 (Add Movement - Right) -> N4 (Add Movement - Forward)
- N13 (Look Input) -> N3 (Add Yaw) -> N2 (Add Pitch)

DATA FLOW ANALYSIS FROM JSON:
- N6.P4 (Control Rotation Z) -> N8.P3 (Forward Vector Input)
- N8.P4 (Forward Vector) -> N4.P4 (Movement Direction)
- N9.P4 (Right Vector) -> N5.P4 (Movement Direction)
- N10.P2 (Control Rotation X) -> N9.P1 (Right Vector Input)
- N10.P4 (Control Rotation Z) -> N9.P3 (Right Vector Input)
- N12.P6 (Move Action X) -> N5.P5 (Scale Value)
- N12.P7 (Move Action Y) -> N4.P5 (Scale Value)
- N13.P6 (Look Action X) -> N3.P4 (Yaw Input)
- N13.P7 (Look Action Y) -> N2.P4 (Pitch Input)
- N14.P5 (Player Controller) -> N16.P1 (Subsystem Target)
- N16.P2 (Enhanced Input Subsystem) -> N15.P3 (Add Context Target)
- N17.P4 (New Controller) -> N14.P4 (Cast Object)
*/