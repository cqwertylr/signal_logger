%% Main body position
mbPosFig = figure();

subplot(3,1,1);
title('Main body position');
grid on; hold on;
xlabel('time [s]'); ylabel('x [m]');
plotDataWithName(logElements, idx_loco_torso_desPositionWorldToBaseInWorldFrame_x, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measPositionWorldToBaseInWorldFrame_x, {'linestyle','-.','color','b'});

subplot(3,1,2);
grid on; hold on;
xlabel('time [s]'); ylabel('y [m]');
plotDataWithName(logElements, idx_loco_torso_desPositionWorldToBaseInWorldFrame_y, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measPositionWorldToBaseInWorldFrame_y, {'linestyle','-.','color','b'});

subplot(3,1,3);
grid on; hold on;
xlabel('time [s]'); ylabel('z [m]');
plotDataWithName(logElements, idx_loco_torso_desPositionWorldToBaseInWorldFrame_z, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measPositionWorldToBaseInWorldFrame_z, {'linestyle','-.','color','b'});

%% Main body orientation
mbRotFig = figure();

subplot(3,1,1);
title('Main body orientation');
grid on; hold on;
xlabel('time [s]'); ylabel('roll [rad]');
plotDataWithName(logElements, idx_loco_torso_desEulerAnglesZyxBaseToWorld_x, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measEulerAnglesZyxBaseToWorld_x, {'linestyle','-.','color','b'});

subplot(3,1,2);
grid on; hold on;
xlabel('time [s]'); ylabel('pitch [rad]');
plotDataWithName(logElements, idx_loco_torso_desEulerAnglesZyxBaseToWorld_y, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measEulerAnglesZyxBaseToWorld_y, {'linestyle','-.','color','b'});

subplot(3,1,3);
grid on; hold on;
xlabel('time [s]'); ylabel('yaw [rad]');
plotDataWithName(logElements, idx_loco_torso_desEulerAnglesZyxBaseToWorld_z, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measEulerAnglesZyxBaseToWorld_z, {'linestyle','-.','color','b'});


%% Main body linear velocity
mbLinVelFig = figure();

subplot(3,1,1);
title('Main body linear velocity');
grid on; hold on;
xlabel('time [s]'); ylabel('roll [rad]');
plotDataWithName(logElements, idx_loco_torso_desiredLinearVelocityBaseInControlFrame_x, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measLinearVelocityBaseInControlFrame_x, {'linestyle','-.','color','b'});

subplot(3,1,2);
grid on; hold on;
xlabel('time [s]'); ylabel('pitch [rad]');
plotDataWithName(logElements, idx_loco_torso_desiredLinearVelocityBaseInControlFrame_y, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measLinearVelocityBaseInControlFrame_y, {'linestyle','-.','color','b'});

subplot(3,1,3);
grid on; hold on;
xlabel('time [s]'); ylabel('yaw [rad]');
plotDataWithName(logElements, idx_loco_torso_desiredLinearVelocityBaseInControlFrame_z, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measLinearVelocityBaseInControlFrame_z, {'linestyle','-.','color','b'});

%% Main body angular velocity
mbRotVelFig = figure();

subplot(3,1,1);
title('Main body angular velocity');
grid on; hold on;
xlabel('time [s]'); ylabel('roll [rad]');
plotDataWithName(logElements, idx_loco_torso_desiredAngularVelocityBaseInControlFrame_x, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measAngularVelocityBaseInControlFrame_x, {'linestyle','-.','color','b'});

subplot(3,1,2);
grid on; hold on;
xlabel('time [s]'); ylabel('pitch [rad]');
plotDataWithName(logElements, idx_loco_torso_desiredAngularVelocityBaseInControlFrame_y, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measAngularVelocityBaseInControlFrame_y, {'linestyle','-.','color','b'});

subplot(3,1,3);
grid on; hold on;
xlabel('time [s]'); ylabel('yaw [rad]');
plotDataWithName(logElements, idx_loco_torso_desiredAngularVelocityBaseInControlFrame_z, {'linestyle','-.','color','r'});
plotDataWithName(logElements, idx_loco_torso_measAngularVelocityBaseInControlFrame_z, {'linestyle','-.','color','b'});


%% Contact force
lfContactForce = figure();

%subplot(3,1,1);
title('Left Fore Contact Force');
grid on; hold on;
xlabel('time [s]'); ylabel('Force [N]');
xlim([30 33]);
ylim([-25 230]);
% plotDataWithName(logElements, idx_loco_leftFore_forceAtEEInWorldFrame_x, {'color','r', 'lineWidth', 2});
% plotDataWithName(logElements, idx_loco_leftFore_forceAtEEInWorldFrame_y, {'color','g', 'lineWidth', 2});
plotDataWithName(logElements, idx_loco_leftFore_forceAtEEInWorldFrame_z, {'color','b', 'lineWidth', 2});
plotDataWithName(logElements, idx_loco_leftFore_desiredForceAtEEInWorldFrame_z, {'color','r', 'lineWidth', 2}, -1.0);

legend('f_{meas}', 'f_{des}');

lhContactForce = figure();

%subplot(3,1,1);
title('Left Hind Contact Force');
grid on; hold on;
xlabel('time [s]'); ylabel('Force [N]');
xlim([30 33]);
ylim([-25 260])
%plotDataWithName(logElements, idx_loco_leftHind_forceAtEEInWorldFrame_x, {'color','r', 'lineWidth', 2});
%plotDataWithName(logElements, idx_loco_leftHind_forceAtEEInWorldFrame_y, {'color','g', 'lineWidth', 2});
plotDataWithName(logElements, idx_loco_leftHind_forceAtEEInWorldFrame_z, {'color','b', 'lineWidth', 2});
plotDataWithName(logElements, idx_loco_leftHind_desiredForceAtEEInWorldFrame_z, {'color','r', 'lineWidth', 2}, -1.0);

% subplot(3,1,2);
% grid on; hold on;
% xlabel('time [s]'); ylabel('pitch [rad]');
% plotDataWithName(logElements, idx_loco_torso_desiredAngularVelocityBaseInControlFrame_y, {'linestyle','-.','color','r'});
% plotDataWithName(logElements, idx_loco_torso_measAngularVelocityBaseInControlFrame_y, {'linestyle','-.','color','b'});
% 
% subplot(3,1,3);
% grid on; hold on;
% xlabel('time [s]'); ylabel('yaw [rad]');
% plotDataWithName(logElements, idx_loco_torso_desiredAngularVelocityBaseInControlFrame_z, {'linestyle','-.','color','r'});
% plotDataWithName(logElements, idx_loco_torso_measAngularVelocityBaseInControlFrame_z, {'linestyle','-.','color','b'});


